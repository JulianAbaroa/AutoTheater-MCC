#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Systems/Director.h"
#include "Core/Threads/InputThread.h"
#include "Core/Threads/TheaterThread.h"
#include <algorithm>
#include <iomanip>
#include <chrono>   
#include <map>

std::vector<DirectorCommand> g_Script;
static float g_LastReplayTime = 0.0f;
bool g_DirectorInitialized = false;
int g_CurrentCommandIndex = 0;

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

void FixOrphanedEvents()
{
	if (g_Timeline.empty()) return;

	const float MAX_GROUP_TIME_DIFF = 0.5f;

	for (int i = 0; i < g_Timeline.size(); i++)
	{
		GameEvent& currentEvent = g_Timeline[i];

		if (currentEvent.Players.empty() && g_OrphanEvents.count(currentEvent.Type) > 0)
		{
			bool foundOwner = 0;

			// Look to the previous GameEvent
			for (int j = i - 1; j >= 0; j--)
			{
				GameEvent& prev = g_Timeline[j];

				float diff = currentEvent.Timestamp - prev.Timestamp;
				if (diff > MAX_GROUP_TIME_DIFF) break;

				if (!prev.Players.empty())
				{
					currentEvent.Players = prev.Players;
					foundOwner = true;
					break;
				}
			}

			// Look to the next GameEvent
			if (!foundOwner)
			{
				for (int j = i + 1; j < g_Timeline.size(); j++)
				{
					GameEvent& next = g_Timeline[j];
					float diff = currentEvent.Timestamp - next.Timestamp;
					if (diff > MAX_GROUP_TIME_DIFF) break;

					if (!next.Players.empty())
					{
						currentEvent.Players = next.Players;
						foundOwner = true;
					}
				}
			}
		}
	}
}

bool ArePlayerSetsEqual(const std::vector<PlayerInfo>& listA, const std::vector<PlayerInfo>& listB)
{
	if (listA.size() != listB.size()) return false;

	for (const auto& playerA : listA)
	{
		bool found = false;
		for (const auto& playerB : listB)
		{
			if (playerA.Id == playerB.Id)
			{
				found = true;
				break;
			}
		}

		if (!found) return false;
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

		for (int i = uniqueEvents.size() - 1; i >= 0; --i)
		{
			const auto& existingEvent = uniqueEvents[i];

			float timeDiff = std::abs(currentEvent.Timestamp - existingEvent.Timestamp);

			if (timeDiff > 1.0f) break;

			if (timeDiff < 0.1f)
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

float GetWeight(const GameEvent& event, const std::string& targetPlayerName)
{
	int weight = 0.0f;
	auto it = g_EventRegistry.find(event.Type);

	if (it != g_EventRegistry.end())
	{
		weight = it->second.Weight;
	}

	return weight;
}

void GenerateScript()
{
	g_Script.clear();
	std::vector<ActionSegment> rawSegments;

	for (size_t i = 0; i < g_Timeline.size(); ++i)
	{
		const auto& currentEvent = g_Timeline[i];
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

		float weight = GetWeight(currentEvent, targetPlayer->Name);
		if (weight == 0.0f) continue;

		ActionSegment segment;
		segment.PlayerName = targetPlayer->Name;
		segment.PlayerID = targetPlayer->Id;

		float rawStart = currentEvent.Timestamp - 6.0f;
		float startTime = (rawStart < 0.0f) ? 0.0f : rawStart;
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

	float lastSegmentEnd = 0.0f;
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
	PrioritizeEvents();
	FixOrphanedEvents();
	RemoveDuplicates();
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

	g_LastReplayTime = 0.0f;
	g_CurrentCommandIndex = 0;
	g_DirectorInitialized = true;
}

void GoToPlayer(uint8_t targetIdx, float nextCommandTimestamp)
{
	if (targetIdx >= 16) return;

	std::stringstream ss;
	ss << "Navigating to player [" << std::to_string(targetIdx) << "]";
	Logger::LogAppend(ss.str().c_str());

	while (g_FollowedPlayerIdx != targetIdx)
	{
		float currentTime = *g_pReplayTime;
		if (currentTime >= nextCommandTimestamp)
		{
			ss.str("");
			ss << "[Director] Next command deadline reached: ("
				<< std::to_string(nextCommandTimestamp) << "s)";
			Logger::LogAppend(ss.str().c_str());
			return;
		}

		uint8_t current = g_FollowedPlayerIdx;
		int forwardSteps = (targetIdx - current + 16) % 16;
		bool movingForward = (forwardSteps > 0 && forwardSteps <= 8);

		if (movingForward) {
			InputThread::NextPlayer();
		}
		else {
			InputThread::PrevPlayer();
		}

		auto startWait = std::chrono::steady_clock::now();
		bool changed = false;

		while (std::chrono::steady_clock::now() - startWait < std::chrono::milliseconds(500))
		{
			if (g_FollowedPlayerIdx != current)
			{
				changed = true;
				break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}

		if (changed)
		{
			bool skipped = false;

			int newForwardSteps = (targetIdx - g_FollowedPlayerIdx + 16) % 16;

			if (movingForward) {
				if (newForwardSteps > 8 && newForwardSteps < 15) skipped = true;
			}
			else {
				if (newForwardSteps <= 8 && newForwardSteps > 1) skipped = true;
			}

			if (skipped)
			{
				ss.str("");
				ss << "WARNING: Player " << (int)targetIdx << " skipped (death). Retrying...";
				Logger::LogAppend(ss.str().c_str());
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
		}
		else
		{
			Logger::LogAppend("WARNING: Retrying input...");
		}
	}

	Logger::LogAppend("[Director] Navigation completed successfully");
}

void Director::Update()
{
	float currentTime = *g_pReplayTime;

	// g_LastReplayTime can produce bugs if it's not reseted correctly
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
				int nextCommandIndex = g_CurrentCommandIndex + 1;
				float deadline =
					(nextCommandIndex < g_Script.size()) ?
					g_Script[nextCommandIndex].Timestamp - 1.0f :
					(currentTime + 36000.0f);

				GoToPlayer(command.TargetPlayerIdx, deadline);

				std::stringstream ss;
				ss << "Execute: Cut to " << (int)command.TargetPlayerIdx << " Reason [" << command.Reason << "]";
				Logger::LogAppend(ss.str().c_str());
			}
		}
		else if (command.Type == CommandType::SetSpeed)
		{
			Theater::SetReplaySpeed(command.SpeedValue);
			
			std::stringstream ss;
			ss << "Execute: Speed set to [x" << command.SpeedValue << "]";
			Logger::LogAppend(ss.str().c_str());
		}

		g_CurrentCommandIndex++;
	}
}