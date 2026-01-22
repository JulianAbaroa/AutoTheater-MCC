#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Core/Systems/Director.h"
#include "Core/Threads/InputThread.h"
#include "Hooks/Data/GetButtonState_Hook.h"
#include "Hooks/Data/SpectatorHandleInput_Hook.h"
#include "Hooks/Data/UpdateTelemetryTimer_Hook.h"
#include <algorithm>
#include <thread>

std::vector<DirectorCommand> g_Script;
std::mutex g_ScriptMutex;

static float g_LastReplayTime = 0.0f;
std::atomic<bool> g_DirectorInitialized{ false };
size_t g_CurrentCommandIndex = 0;

static float DIRECTOR_WARMUP_TIME = 5.0f;

void PrioritizeEvents()
{
	std::sort(g_Timeline.begin(), g_Timeline.end(), [](const GameEvent& a, const GameEvent& b)
	{
		if (abs(a.Timestamp - b.Timestamp) < 0.001f)
		{
			return a.Players.size() > b.Players.size();
		}
	
		return a.Timestamp < b.Timestamp;
	});
}

bool ArePlayerSetsEqual(const std::vector<PlayerInfo>& listA, const std::vector<PlayerInfo>& listB)
{
	if (listA.size() != listB.size()) return false;

	for (size_t i = 0; i < listA.size(); ++i)
	{
		if (listA[i].Id != listB[i].Id) return false;
	}

	return true;
}

void RemoveDuplicates()
{
	if (g_Timeline.empty()) return;

	std::sort(g_Timeline.begin(), g_Timeline.end(), [](const GameEvent& a, const GameEvent& b) {
		return a.Timestamp < b.Timestamp;
	});

	std::vector<GameEvent> uniqueEvents;

	for (const auto& currentEvent : g_Timeline)
	{
		bool isDuplicate = false;

		for (auto i = std::ssize(uniqueEvents) - 1; i >= 0; --i)
		{
			const auto& existingEvent = uniqueEvents[i];

			float timeDiff = std::abs(currentEvent.Timestamp - existingEvent.Timestamp);

			if (timeDiff > 1.0f) break;

			if (timeDiff <= 1.0f)
			{
				if (currentEvent.Type == existingEvent.Type)
				{
					if (ArePlayerSetsEqual(currentEvent.Players, existingEvent.Players))
					{
						isDuplicate = true;

						break;
					}
				}
			}
		}

		if (!isDuplicate)
		{
			uniqueEvents.push_back(currentEvent);
		}
	}

	g_Timeline = uniqueEvents;
}

void OptimizeSegments(std::vector<ActionSegment>& segments)
{
	if (segments.empty()) return;

	std::sort(segments.begin(), segments.end(), [](const ActionSegment& a, const ActionSegment& b) {
		return a.StartTime < b.StartTime;
	});

	std::vector<ActionSegment> merged;
	merged.push_back(segments[0]);

	for (size_t i = 1; i < segments.size(); i++)
	{
		ActionSegment& current = segments[i];
		ActionSegment& previous = merged.back();

		float originalStartTime = current.StartTime;
		float estimatedEventTimestamp = originalStartTime + 6.0f;

		bool overlaps = current.StartTime <= (previous.EndTime + 4.0f);

		if (overlaps)
		{
			if (previous.PlayerName == current.PlayerName)
			{
				previous.EndTime = (std::max)(previous.EndTime, current.EndTime);
				previous.TotalScore += current.TotalScore;
				previous.TotalEvents.insert(
					previous.TotalEvents.end(),
					current.TotalEvents.begin(),
					current.TotalEvents.end()
				);
			}
			else
			{
				float previousDurationSoFar = current.StartTime - previous.StartTime;

				float scoreMultiplier = 1.3f;
				if (previous.TotalScore > 50.0f) scoreMultiplier = 2.5f;
				else if (previous.TotalScore > 30.0f) scoreMultiplier = 1.8f;

				if (previousDurationSoFar > 3.0f && current.TotalScore > (previous.TotalScore * scoreMultiplier))
				{
					if (current.StartTime < previous.EndTime)
					{
						previous.EndTime = current.StartTime;	
					}

					previous.EndTime = current.StartTime;
					merged.push_back(current);
				}
				else
				{
					float proposedNewStartTime = previous.EndTime;
					float remainingPrePoll = estimatedEventTimestamp - proposedNewStartTime;

					const float MIN_PREROLL_NEEDED = 4.0f;

					if (remainingPrePoll >= MIN_PREROLL_NEEDED)
					{
						current.StartTime = proposedNewStartTime;

						if (current.EndTime - current.StartTime > 5.0f)
						{
							merged.push_back(current);
						}
					}
				}
			}
		}
		else
		{
			merged.push_back(current);
		}
	}

	std::vector<ActionSegment> finalSegments;
	for (const auto& seg : merged)
	{
		if ((seg.EndTime - seg.StartTime) >= 4.0f)
		{
			finalSegments.push_back(seg);
		}
	}
	segments = finalSegments;
}

int GetWeight(const GameEvent& event)
{
	for (const auto& entry : g_EventRegistry)
	{
		if (entry.second.Type== event.Type)
		{
			return entry.second.Weight;
		}
	}

	return 0;
}

void GenerateScript()
{
	std::vector<ActionSegment> rawSegments;

	for (size_t i = 0; i < g_Timeline.size(); ++i)
	{
		const auto& currentEvent = g_Timeline[i];
		if (currentEvent.Timestamp < DIRECTOR_WARMUP_TIME + 2.0f) continue;
		if (currentEvent.Players.empty()) continue;

		const PlayerInfo* targetPlayer = &currentEvent.Players[0];
		if (currentEvent.Players.size() >= 2)
		{
			for (const auto& player : currentEvent.Players)
			{
				if (!player.IsVictim)
				{
					targetPlayer = &player;
					break;
				}
			}
		}

		int weight = GetWeight(currentEvent);
		if (weight == 0) continue;

		ActionSegment segment;
		segment.PlayerName = targetPlayer->Name;
		segment.PlayerID = targetPlayer->Id;

		float rawStart = currentEvent.Timestamp - 6.0f;
		float startTime = (rawStart < DIRECTOR_WARMUP_TIME) ? DIRECTOR_WARMUP_TIME : rawStart;
		float endTime = currentEvent.Timestamp + 2.0f;

		float lookAheadWindow = 8.0f;
		for (size_t j = i + 1; j < g_Timeline.size(); ++j)
		{
			const auto& nextEvent = g_Timeline[j];
			if (nextEvent.Timestamp - currentEvent.Timestamp > lookAheadWindow) break;
		
			bool isInvolved = false;
			for (const auto& player : nextEvent.Players)
			{
				if (player.Id == targetPlayer->Id) { 
					isInvolved = true; break; 
				}
			}
		
			if (isInvolved)
			{
				float newEndTime = nextEvent.Timestamp + 2.0f;
				if (newEndTime > endTime)
				{
					endTime = newEndTime;
				}
			}
		}

		segment.StartTime = startTime;
		segment.EndTime = endTime;
		if (segment.EndTime - segment.StartTime < 5.0f) continue;

		segment.TotalScore = weight;
		segment.TotalEvents.push_back(currentEvent.Type);
		rawSegments.push_back(segment);
	}

	OptimizeSegments(rawSegments);

	float lastSegmentEnd = DIRECTOR_WARMUP_TIME;
	for (const auto& segment : rawSegments)
	{
		float gap = segment.StartTime - lastSegmentEnd;

		if (gap > 5.0f)
		{
			DirectorCommand speedCmd;
			speedCmd.Timestamp = lastSegmentEnd + 0.25f;
			speedCmd.Type = CommandType::SetSpeed;
			speedCmd.SpeedValue = (gap > 10.0f) ? 4.0f : 2.0f;
			g_Script.push_back(speedCmd);

			DirectorCommand normalCmd;
			normalCmd.Timestamp = segment.StartTime - 0.25f;
			normalCmd.Type = CommandType::SetSpeed;
			normalCmd.SpeedValue = 1.0f;
			g_Script.push_back(normalCmd);
		}

		DirectorCommand cutCmd;
		cutCmd.Timestamp = segment.StartTime - 0.25f;
		cutCmd.Type = CommandType::Cut;
		cutCmd.TargetPlayerIdx = segment.PlayerID;
		cutCmd.TargetPlayerName = segment.PlayerName;

		std::stringstream ss;
		ss << "Score: " << segment.TotalScore << " Events: " << segment.TotalEvents.size();
		cutCmd.Reason = ss.str();

		lastSegmentEnd = segment.EndTime;
		g_Script.push_back(cutCmd);
	}

	std::sort(g_Script.begin(), g_Script.end(), [](const DirectorCommand& a, const DirectorCommand& b) {
		return a.Timestamp < b.Timestamp;
	});
}

void Director::Initialize()
{
	std::lock_guard<std::mutex> lock(g_TimelineMutex);

	PrioritizeEvents();
	RemoveDuplicates();

	{
		std::lock_guard<std::mutex> lock(g_ScriptMutex);

		g_Script.clear();
		GenerateScript();

		Logger::LogAppend("=== Generated Script ===");
		for (const auto& cmd : g_Script) {
			std::stringstream ss;

			int totalSeconds = static_cast<int>(cmd.Timestamp);
			int minutes = totalSeconds / 60;
			int seconds = totalSeconds % 60;

			ss << "["
				<< std::setw(2) << std::setfill('0') << minutes << ":"
				<< std::setw(2) << std::setfill('0') << seconds
				<< "] ";

			if (cmd.Type == CommandType::Cut) {
				ss << "Cut to: Player " << cmd.TargetPlayerName << " (" << cmd.Reason << ")";
			}
			else {
				ss << "Speed: " << cmd.SpeedValue << "x";
			}

			Logger::LogAppend(ss.str().c_str());
		}
		Logger::LogAppend("======================");
		
		g_CurrentCommandIndex = 0;
	}

	g_LastReplayTime = 0.0f;
	g_DirectorInitialized.store(true);
}

static void PushInput(InputAction action, InputContext context)
{
	std::lock_guard<std::mutex> lock(g_InputQueueMutex);
	if (g_InputQueue.size() > 1) return;
	g_InputQueue.push({ action, context });
}

void GoToPlayer(uint8_t targetIdx, float nextCommandTimestamp)
{
	if (targetIdx >= 16) return;

	Logger::LogAppend(("[Director] Starting navigation to: " + std::to_string(targetIdx)).c_str());

	while (g_FollowedPlayerIdx != targetIdx)
	{
		if (*g_pReplayTime >= nextCommandTimestamp)
		{
			Logger::LogAppend("[Director] Navigation aborted: Time limit reached.");
			return;
		}
		
		uint8_t current = g_FollowedPlayerIdx;
		int forwardSteps = (targetIdx - current + 16) % 16;
		bool movingForward = (forwardSteps > 0 && forwardSteps <= 8);

		PushInput(movingForward ? InputAction::NextPlayer : InputAction::PreviousPlayer, InputContext::Theater);

		auto startWait = std::chrono::steady_clock::now();
		while (!g_InputProcessing.load() && (std::chrono::steady_clock::now() - startWait < std::chrono::milliseconds(100)))
		{
			std::this_thread::yield();
		}

		while (g_InputProcessing.load()) { std::this_thread::yield(); }

		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}

	Logger::LogAppend("[Director] Navigation successful.");
}

float SafeGetCurrentTime()
{
	__try
	{
		return *g_pReplayTime;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return 0.0f;
	}
}

void Director::Update()
{
	if (!g_pReplayTime || g_CurrentPhase != LibrarianPhase::ExecuteDirector) {
		return;
	}

	if (g_pReplayModule != 0 && g_CameraAttached == 0x01)
	{
		PushInput(InputAction::ToggleFreecam, InputContext::Theater);
	}

	std::lock_guard<std::mutex> lock(g_ScriptMutex);

	if (g_Script.empty()) return;

	// g_LastReplayTime can produce bugs if it's not reseted correctly
	float currentTime = SafeGetCurrentTime();

	if (*g_pReplayTime < DIRECTOR_WARMUP_TIME) return;

	if (currentTime < g_LastReplayTime)
	{
		auto it = std::lower_bound(
			g_Script.begin(),
			g_Script.end(),
			currentTime,
			[](const DirectorCommand& cmd, float timeVal) {
				return cmd.Timestamp < timeVal;
			}
		);

		g_CurrentCommandIndex = std::distance(g_Script.begin(), it);

		std::stringstream ss;
		ss << "[Director] Rewind detected at " << currentTime << "s. Resetting script to index " << g_CurrentCommandIndex;
		Logger::LogAppend(ss.str().c_str());
	}

	g_LastReplayTime = currentTime;

	if (g_CurrentCommandIndex >= g_Script.size()) return;

	DirectorCommand& command = g_Script[g_CurrentCommandIndex];

	if (currentTime >= command.Timestamp)
	{
		if (command.Type == CommandType::Cut)
		{
			if (command.TargetPlayerIdx != g_FollowedPlayerIdx)
			{
				std::stringstream ss;
				ss << "Execute: Cut to " << (int)command.TargetPlayerIdx << " Reason [" << command.Reason << "]";
				Logger::LogAppend(ss.str().c_str());

				size_t nextCommandIndex = g_CurrentCommandIndex + 1;
				float deadline =
					(nextCommandIndex < g_Script.size()) ?
					g_Script[nextCommandIndex].Timestamp - 1.0f :
					(currentTime + 36000.0f);

				GoToPlayer(command.TargetPlayerIdx, deadline);
			}
		}
		else if (command.Type == CommandType::SetSpeed)
		{
			std::stringstream ss;
			ss << "Execute: Speed set to [x" << command.SpeedValue << "]";
			Logger::LogAppend(ss.str().c_str());

			Theater::SetReplaySpeed(command.SpeedValue);
		}

		g_CurrentCommandIndex++;
	}
}