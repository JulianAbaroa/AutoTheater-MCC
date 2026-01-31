#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Systems/Theater.h"
#include "Core/Systems/Director.h"
#include "Core/Common/GlobalState.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>

using namespace std::chrono_literals;

static const float DIRECTOR_WARMUP_TIME = 5.0f;

static void PrioritizeEvents(std::vector<GameEvent> tempTimeline)
{
	std::sort(tempTimeline.begin(), tempTimeline.end(), [](const GameEvent& a, const GameEvent& b)
	{
		if (abs(a.Timestamp - b.Timestamp) < 0.001f)
		{
			return a.Players.size() > b.Players.size();
		}
	
		return a.Timestamp < b.Timestamp;
	});

	std::lock_guard lock(g_pState->TimelineMutex);
	g_pState->Timeline = tempTimeline;
}

static bool ArePlayerSetsEqual(const std::vector<PlayerInfo>& listA, const std::vector<PlayerInfo>& listB)
{
	if (listA.size() != listB.size()) return false;

	for (size_t i = 0; i < listA.size(); ++i)
	{
		if (listA[i].Id != listB[i].Id) return false;
	}

	return true;
}

static void RemoveDuplicates(std::vector<GameEvent> tempTimeline)
{
	if (tempTimeline.empty()) return;

	std::sort(tempTimeline.begin(), tempTimeline.end(), [](const GameEvent& a, const GameEvent& b) {
		return a.Timestamp < b.Timestamp;
	});

	std::vector<GameEvent> uniqueEvents;

	for (const auto& currentEvent : tempTimeline)
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

	std::lock_guard lock(g_pState->TimelineMutex);
	g_pState->Timeline = uniqueEvents;
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

static int GetWeight(const GameEvent& event, const std::unordered_map<std::wstring, EventInfo>& localRegistry)
{
	for (const auto& entry : localRegistry)
	{
		if (entry.second.Type== event.Type)
		{
			return entry.second.Weight;
		}
	}

	return 0;
}

static void GenerateScript(std::vector<GameEvent> tempTimeline)
{
	std::unordered_map<std::wstring, EventInfo> registryCopy;
	{
		std::lock_guard lock(g_pState->ConfigMutex);
		registryCopy = g_pState->EventRegistry;
	}

	std::vector<ActionSegment> rawSegments;

	for (size_t i = 0; i < tempTimeline.size(); ++i)
	{
		const auto& currentEvent = tempTimeline[i];
		if (currentEvent.Timestamp < DIRECTOR_WARMUP_TIME + 2.0f) continue;
		if (currentEvent.Players.empty()) continue;
	
		const PlayerInfo* targetPlayer = &currentEvent.Players[0];
	
		int weight = GetWeight(currentEvent, registryCopy);
		if (weight == 0) continue;
	
		ActionSegment segment;
		segment.PlayerName = targetPlayer->Name;
		segment.PlayerID = targetPlayer->Id;
	
		float rawStart = currentEvent.Timestamp - 4.0f;
		float startTime = (rawStart < DIRECTOR_WARMUP_TIME) ? DIRECTOR_WARMUP_TIME : rawStart;
		float endTime = currentEvent.Timestamp + 4.0f;
		
		float lookAheadWindow = 8.0f;
		for (size_t j = i + 1; j < tempTimeline.size(); ++j)
		{
			const auto& nextEvent = tempTimeline[j];
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
				float newEndTime = nextEvent.Timestamp + 4.0f;
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

	std::vector<DirectorCommand> tempScript;

	for (const auto& segment : rawSegments)
	{
		float gap = segment.StartTime - lastSegmentEnd;
	
		if (gap > 5.0f)
		{
			DirectorCommand speedCmd;
			speedCmd.Timestamp = lastSegmentEnd + 0.25f;
			speedCmd.Type = CommandType::SetSpeed;
			speedCmd.SpeedValue = (gap > 10.0f) ? 4.0f : 2.0f;

			tempScript.push_back(speedCmd);
	
			DirectorCommand normalCmd;
			normalCmd.Timestamp = segment.StartTime - 0.25f;
			normalCmd.Type = CommandType::SetSpeed;
			normalCmd.SpeedValue = 1.0f;

			tempScript.push_back(normalCmd);
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

		tempScript.push_back(cutCmd);
	}
	
	std::sort(tempScript.begin(), tempScript.end(), [](const DirectorCommand& a, const DirectorCommand& b) {
		return a.Timestamp < b.Timestamp;
	});

	std::lock_guard lock(g_pState->DirectorMutex);
	g_pState->Script = tempScript;
}

void Director::Initialize()
{
	std::vector<GameEvent> timelineCopy;
	{
		std::lock_guard lock(g_pState->TimelineMutex);
		timelineCopy = g_pState->Timeline;
	}

	PrioritizeEvents(timelineCopy);
	RemoveDuplicates(timelineCopy);

	{
		std::lock_guard lock(g_pState->DirectorMutex);
		g_pState->Script.clear();
	}

	GenerateScript(timelineCopy);

	Logger::LogAppend("=== Generated Script ===");

	std::vector<DirectorCommand> tempScript{};

	{
		std::lock_guard lock(g_pState->DirectorMutex);
		tempScript = g_pState->Script;
	}

	for (const auto& cmd : tempScript) {
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
	Logger::LogAppend("=== Script End ===");
	
	std::lock_guard lock(g_pState->DirectorMutex);
	g_pState->CurrentCommandIndex.store(0);
	g_pState->LastReplayTime.store(0.0f);
	g_pState->DirectorInitialized.store(true);
}

static void PushInput(InputAction action, InputContext context)
{
	std::lock_guard<std::mutex> lock(g_pState->InputMutex);
	if (g_pState->InputQueue.size() > 1) return;
	g_pState->InputQueue.push({ action, context });
}

static void GoToPlayer(uint8_t targetIdx, float nextCommandTimestamp)
{
	if (targetIdx >= 16) return;

	Logger::LogAppend(("[Director] Starting navigation to: " + std::to_string(targetIdx)).c_str());

	auto getFollowedIdx = [&]() {
		return g_pState->FollowedPlayerIdx.load();
	};

	while (getFollowedIdx() != targetIdx)
	{
		if (*g_pState->pReplayTime.load() >= nextCommandTimestamp)
		{
			Logger::LogAppend("[Director] Navigation aborted: Time limit reached.");
			return;
		}
		
		uint8_t current = g_pState->FollowedPlayerIdx.load();
		int forwardSteps = (targetIdx - current + 16) % 16;
		bool movingForward = (forwardSteps > 0 && forwardSteps <= 8);

		PushInput(movingForward ? InputAction::NextPlayer : InputAction::PreviousPlayer, InputContext::Theater);

		auto startWait = std::chrono::steady_clock::now();
		
		while (!g_pState->InputProcessing.load() && (std::chrono::steady_clock::now() - startWait < 100ms))
		{
			std::this_thread::yield();
		}

		while (g_pState->InputProcessing.load()) { std::this_thread::yield(); }

		std::this_thread::sleep_for(30ms);
	}

	Logger::LogAppend("[Director] Navigation successful.");
}

static float SafeGetCurrentTime()
{
	__try
	{
		return *g_pState->pReplayTime.load();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return 0.0f;
	}
}

void Director::Update()
{
	if (!g_pState->pReplayTime.load() || g_pState->CurrentPhase.load() != AutoTheaterPhase::Director) {
		return;
	}

	if (g_pState->pReplayModule.load() != 0 && 
		g_pState->CameraAttached.load() == 0x01 &&
		g_pState->AttachThirdPersonPOV.load())
	{
		PushInput(InputAction::ToggleFreecam, InputContext::Theater);
	}

	std::vector<DirectorCommand> scriptCopy{};

	{
		std::lock_guard<std::mutex> lock(g_pState->DirectorMutex);
		scriptCopy = g_pState->Script;
	}

	if (scriptCopy.empty()) return;

	// g_LastReplayTime can produce bugs if it's not reseted correctly
	float currentTime = SafeGetCurrentTime();

	float* pTime = g_pState->pReplayTime.load();
	if (!pTime || *pTime < DIRECTOR_WARMUP_TIME) return;

	if (currentTime < g_pState->LastReplayTime.load())
	{
		auto it = std::lower_bound(
			scriptCopy.begin(),
			scriptCopy.end(),
			currentTime,
			[](const DirectorCommand& cmd, float timeVal) {
				return cmd.Timestamp < timeVal;
			}
		);

		g_pState->CurrentCommandIndex.store(std::distance(scriptCopy.begin(), it));

		std::stringstream ss;
		ss << "[Director] Rewind detected at " << currentTime << "s. Resetting script to index " << g_pState->CurrentCommandIndex.load();
		Logger::LogAppend(ss.str().c_str());
	}

	g_pState->LastReplayTime.store(currentTime);

	if (g_pState->CurrentCommandIndex.load() >= scriptCopy.size()) return;

	DirectorCommand& command = scriptCopy[g_pState->CurrentCommandIndex.load()];

	if (currentTime >= command.Timestamp)
	{
		if (command.Type == CommandType::Cut)
		{
			if (command.TargetPlayerIdx != g_pState->FollowedPlayerIdx.load())
			{
				std::stringstream ss;
				ss << "Execute: Cut to " << (int)command.TargetPlayerIdx << " Reason [" << command.Reason << "]";
				Logger::LogAppend(ss.str().c_str());

				size_t nextCommandIndex = g_pState->CurrentCommandIndex.load() + 1;
				float deadline =
					(nextCommandIndex < scriptCopy.size()) ?
					scriptCopy[nextCommandIndex].Timestamp - 1.0f :
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

		g_pState->CurrentCommandIndex++;
	}
}