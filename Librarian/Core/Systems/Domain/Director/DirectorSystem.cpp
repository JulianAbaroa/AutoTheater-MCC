#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Director/DirectorState.h"
#include "Core/States/Domain/Director/EventRegistryState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Domain/Timeline/TimelineState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Engine/InputState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Director/DirectorSystem.h"
#include "Core/Systems/Domain/Theater/TheaterSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <algorithm>
#include <chrono>

using namespace std::chrono_literals;

void DirectorSystem::Initialize()
{
	std::vector<GameEvent> timelineCopy = g_pState->Domain->Timeline->GetTimelineCopy();

	PrioritizeEvents(timelineCopy);
	RemoveDuplicates(timelineCopy);

	g_pState->Domain->Director->ClearScript();

	if (!timelineCopy.empty()) this->GenerateScript(timelineCopy);
	
	g_pSystem->Domain->Director->SetCurrentCommandIndex(0);
	g_pSystem->Domain->Director->SetLastReplayTime(0.0f);
	g_pState->Domain->Director->SetInitialized(true);

	g_pSystem->Debug->Log("[DirectorSystem] INFO: Director initialized.");
}

void DirectorSystem::Update()
{
	if (!g_pState->Domain->Theater->GetTimePtr() || g_pState->Infrastructure->Lifecycle->GetCurrentPhase() != Phase::Director) 
	{
		return;
	}

	if (g_pState->Domain->Theater->GetReplayModule() != 0 && 
		g_pState->Domain->Theater->GetCameraMode() == 0x01 &&
		g_pState->Domain->Theater->IsThirdPersonForced()) 
	{
		InputRequest request{};
		request.Context = InputContext::Theater;
		request.Action = InputAction::ToggleFreecam;
		g_pState->Infrastructure->Input->EnqueueRequest(request, true);
	}

	std::vector<DirectorCommand> scriptCopy = g_pState->Domain->Director->GetScriptCopy();;
	if (scriptCopy.empty()) return;

	// g_LastReplayTime can produce bugs if it's not reseted correctly
	float currentTime = SafeGetCurrentTime();
	float* pTime = g_pState->Domain->Theater->GetTimePtr();
	if (!pTime || *pTime < m_DirectorWarmupTime) return;

	if (currentTime < g_pSystem->Domain->Director->GetLastReplayTime())
	{
		auto it = std::lower_bound(
			scriptCopy.begin(),
			scriptCopy.end(),
			currentTime,
			[](const DirectorCommand& cmd, float timeVal) {
				return cmd.Timestamp < timeVal;
			});

		g_pSystem->Domain->Director->SetCurrentCommandIndex(std::distance(scriptCopy.begin(), it));
		g_pSystem->Debug->Log("[DirectorSystem] INFO: Rewind detected at %.2fs. Resetting script to index %d",
			currentTime, g_pSystem->Domain->Director->GetCurrentCommandIndex());
	}
	g_pSystem->Domain->Director->SetLastReplayTime(currentTime);

	size_t currentIndex = g_pSystem->Domain->Director->GetCurrentCommandIndex();
	if (currentIndex >= scriptCopy.size())
	{
		if (g_pState->Infrastructure->FFmpeg->StopOnLastEvent() && g_pState->Infrastructure->FFmpeg->IsRecording())
		{
			if (m_StopDelayStartTime == 0.0f)
			{
				m_StopDelayStartTime = currentTime;
				g_pSystem->Debug->Log("[DirectorSystem] INFO: Script finished, waiting %.2f seconds before stop...", g_pState->Infrastructure->FFmpeg->GetStopDelayDuration());
			}

			if (currentTime - m_StopDelayStartTime >= g_pState->Infrastructure->FFmpeg->GetStopDelayDuration())
			{
				g_pSystem->Debug->Log("[DirectorSystem] WARNING: Delay finished, stopping recording.");
				g_pSystem->Infrastructure->FFmpeg->Stop();
				m_StopDelayStartTime = 0.0f;
			}
		}

		return;
	}

	DirectorCommand& command = scriptCopy[g_pSystem->Domain->Director->GetCurrentCommandIndex()];

	if (currentTime >= command.Timestamp)
	{
		if (command.Type == CommandType::Cut)
		{
			if (command.TargetPlayerIdx != g_pState->Domain->Theater->GetSpectatedPlayerIndex())
			{
				g_pSystem->Debug->Log("[DirectorSystem] INFO: Execute: Cut to %d Reason [%s]", command.TargetPlayerIdx, command.Reason.c_str());

				size_t nextCommandIndex = g_pSystem->Domain->Director->GetCurrentCommandIndex() + 1;
				float deadline =
					(nextCommandIndex < scriptCopy.size()) ?
					scriptCopy[nextCommandIndex].Timestamp - 1.0f :
					(currentTime + 36000.0f);

				GoToPlayer(command.TargetPlayerIdx, deadline);
			}
		}
		else if (command.Type == CommandType::SetSpeed)
		{
			g_pSystem->Debug->Log("[DirectorSystem] INFO: Speed set to [%.2fx]", command.SpeedValue);
			g_pSystem->Domain->Theater->SetReplaySpeed(command.SpeedValue);
		}

		g_pSystem->Domain->Director->IncrementCurrentCommandIndex();
	}
}


float DirectorSystem::GetLastReplayTime() const { return m_LastReplayTime.load(); }
void DirectorSystem::SetLastReplayTime(float lastReplayTime) { m_LastReplayTime.store(lastReplayTime); }

size_t DirectorSystem::GetCurrentCommandIndex() const { return m_CurrentCommandIndex.load(); }
void DirectorSystem::SetCurrentCommandIndex(size_t cmdIndex) { m_CurrentCommandIndex.store(cmdIndex); }

void DirectorSystem::IncrementCurrentCommandIndex()
{
	size_t scriptSize = g_pState->Domain->Director->GetScriptSize();
	size_t current = m_CurrentCommandIndex.load();

	if (current < scriptSize)
	{
		m_CurrentCommandIndex.store(current + 1);
	}
}


void DirectorSystem::GoToPlayer(uint8_t targetIdx, float nextCommandTimestamp)
{
	if (targetIdx >= 16) return;

	g_pSystem->Debug->Log("[DirectorSystem] INFO: Starting navigation to: %d", targetIdx);

	while (g_pState->Domain->Theater->GetSpectatedPlayerIndex() != targetIdx)
	{
		if (*g_pState->Domain->Theater->GetTimePtr() >= nextCommandTimestamp)
		{
			g_pSystem->Debug->Log("[DirectorSystem] WARNING: Navigation aborted, time limit reached.");
			return;
		}

		uint8_t current = g_pState->Domain->Theater->GetSpectatedPlayerIndex();
		int forwardSteps = (targetIdx - current + 16) % 16;
		bool movingForward = (forwardSteps > 0 && forwardSteps <= 8);

		InputRequest request{};
		request.Context = InputContext::Theater;
		request.Action = movingForward ? InputAction::NextPlayer : InputAction::PreviousPlayer;
		g_pState->Infrastructure->Input->EnqueueRequest(request, true);

		auto startWait = std::chrono::steady_clock::now();

		while (!g_pState->Infrastructure->Input->IsProcessing() && (std::chrono::steady_clock::now() - startWait < 100ms))
		{
			std::this_thread::yield();
		}

		while (g_pState->Infrastructure->Input->IsProcessing()) { std::this_thread::yield(); }

		std::this_thread::sleep_for(30ms);
	}

	g_pSystem->Debug->Log("[DirectorSystem] INFO: Navigation successful.");
}


void DirectorSystem::GenerateScript(std::vector<GameEvent> timeline)
{
	auto registryCopy = g_pState->Domain->EventRegistry->GetEventRegistryCopy();

	std::vector<ActionSegment> rawSegments;

	for (size_t i = 0; i < timeline.size(); ++i)
	{
		const auto& currentEvent = timeline[i];
		if (currentEvent.Timestamp < m_DirectorWarmupTime + 2.0f) continue;
		if (currentEvent.Players.empty()) continue;

		const PlayerInfo* targetPlayer = &currentEvent.Players[0];

		int weight = GetWeight(currentEvent, registryCopy);
		if (weight == 0) continue;

		ActionSegment segment;
		segment.PlayerName = targetPlayer->Name;
		segment.PlayerID = targetPlayer->Id;

		float rawStart = currentEvent.Timestamp - 4.0f;
		float startTime = (rawStart < m_DirectorWarmupTime) ? m_DirectorWarmupTime : rawStart;
		float endTime = currentEvent.Timestamp + 4.0f;

		float lookAheadWindow = 8.0f;
		for (size_t j = i + 1; j < timeline.size(); ++j)
		{
			const auto& nextEvent = timeline[j];
			if (nextEvent.Timestamp - currentEvent.Timestamp > lookAheadWindow) break;

			bool isInvolved = false;
			for (const auto& player : nextEvent.Players)
			{
				if (player.Id == targetPlayer->Id) 
				{
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

	float lastSegmentEnd = m_DirectorWarmupTime;

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

		char buffer[128]{};
		snprintf(buffer, sizeof(buffer), "Score: %d Events: %zu", 
			segment.TotalScore, segment.TotalEvents.size());
		cutCmd.Reason = buffer;

		lastSegmentEnd = segment.EndTime;

		tempScript.push_back(cutCmd);
	}

	std::sort(tempScript.begin(), tempScript.end(), [](const DirectorCommand& a, const DirectorCommand& b) {
		return a.Timestamp < b.Timestamp;
	});

	g_pState->Domain->Director->SetScript(tempScript);

	g_pSystem->Debug->Log("[DirectorSystem] INFO: Script generated.");
}

void DirectorSystem::OptimizeSegments(std::vector<ActionSegment>& segments)
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

int DirectorSystem::GetWeight(const GameEvent& event, const std::unordered_map<std::wstring, EventInfo>& localRegistry)
{
	for (const auto& entry : localRegistry)
	{
		if (entry.second.Type == event.Type)
		{
			return entry.second.Weight;
		}
	}

	return 0;
}


void DirectorSystem::PrioritizeEvents(std::vector<GameEvent> timeline)
{
	std::sort(timeline.begin(), timeline.end(), [](const GameEvent& a, const GameEvent& b)
		{
			if (abs(a.Timestamp - b.Timestamp) < 0.001f)
			{
				return a.Players.size() > b.Players.size();
			}

			return a.Timestamp < b.Timestamp;
		});

	g_pState->Domain->Timeline->SetTimeline(timeline);
}

bool DirectorSystem::ArePlayerSetsEqual(const std::vector<PlayerInfo>& listA, const std::vector<PlayerInfo>& listB)
{
	if (listA.size() != listB.size()) return false;

	for (size_t i = 0; i < listA.size(); ++i)
	{
		if (listA[i].Id != listB[i].Id) return false;
	}

	return true;
}

void DirectorSystem::RemoveDuplicates(std::vector<GameEvent> timeline)
{
	if (timeline.empty()) return;

	std::sort(timeline.begin(), timeline.end(), [](const GameEvent& a, const GameEvent& b) {
		return a.Timestamp < b.Timestamp;
	});

	std::vector<GameEvent> uniqueEvents;

	for (const auto& currentEvent : timeline)
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

	g_pState->Domain->Timeline->SetTimeline(uniqueEvents);
}

float DirectorSystem::SafeGetCurrentTime()
{
	__try
	{
		return *g_pState->Domain->Theater->GetTimePtr();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return 0.0f;
	}
}