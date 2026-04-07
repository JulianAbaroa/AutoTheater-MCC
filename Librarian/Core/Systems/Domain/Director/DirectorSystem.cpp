#include "pch.h"
#include "Core/Common/Types/BlamTypes.h"
#include "Core/UI/CoreUI.h"
#include "Core/UI/Tabs/Primary/DirectorTab.h"
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
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Infrastructure/CaptureThread.h"
#include <algorithm>
#include <chrono>

using namespace std::chrono_literals;

void DirectorSystem::Initialize()
{
	std::vector<GameEvent> timelineCopy = g_pState->Domain->Timeline->GetTimelineCopy();
	if (timelineCopy.empty())
	{
		g_pSystem->Debug->Log("[DirectorSystem] INFO: Timeline empty, skipping director initialization.");
		g_pState->Domain->Director->SetSkipped(true);
		return;
	}

	// Prepare for the script creation.
	this->PrioritizeEvents(timelineCopy);
	this->RemoveDuplicates(timelineCopy);

	// Create the script.
	this->GenerateScript(timelineCopy);

	g_pState->Domain->Director->SetInitialized(true);
	g_pSystem->Debug->Log("[DirectorSystem] INFO: Director initialized.");
}

void DirectorSystem::Update()
{
	float* timePtr = g_pState->Domain->Theater->GetTimePtr();
	auto currentPhase = g_pState->Infrastructure->Lifecycle->GetCurrentPhase();
	if (!timePtr || currentPhase != Phase::Director) return;

	if (!m_ScriptCached)
	{
		m_CachedScript = g_pState->Domain->Director->GetScriptCopy();
		m_ScriptCached = true;
	}
	if (m_CachedScript.empty()) return;

	float currentTime = this->SafeGetCurrentTime();
	ReplayModule replayModule = g_pState->Domain->Theater->GetModuleSnapshot();
	this->ForceSpectatorModes(replayModule);

	if (*timePtr < m_DirectorWarmupTime) return;

	float lastTime = this->GetLastReplayTime();
	float timeDelta = currentTime - lastTime;

	float replaySpeed = *g_pState->Domain->Theater->GetTimeScalePtr();
	float maxExpectedDelta = (std::max)(replaySpeed * 2.0f, 5.0f);
	bool isJump = (timeDelta < 0.0f || timeDelta > maxExpectedDelta);

	if (isJump)
	{
		auto it = std::lower_bound(
			m_CachedScript.begin(), m_CachedScript.end(), currentTime,
			[](const DirectorCommand& cmd, float timeVal) {
				return cmd.Timestamp < timeVal;
			});
		size_t newIndex = std::distance(m_CachedScript.begin(), it);
		this->SetCurrentCommandIndex(newIndex);

		const char* jumpType = (timeDelta < 0.0f) ? "Rewind" : "Fast-Forward";
		g_pSystem->Debug->Log("[DirectorSystem] INFO: %s detected (%.2fs -> %.2fs). Resyncing index to %zu",
			jumpType, lastTime, currentTime, newIndex);

		for (int i = (int)newIndex - 1; i >= 0; i--)
		{
			if (m_CachedScript[i].Type == CommandType::SetSpeed)
			{
				g_pSystem->Domain->Theater->SetReplaySpeed(m_CachedScript[i].SpeedValue);
				g_pSystem->Debug->Log("[DirectorSystem] INFO: Resync applied last SetSpeed: %.2fx",
					m_CachedScript[i].SpeedValue);
				break;
			}
		}

		m_StopDelayStartTime = 0.0f;
	}

	this->SetLastReplayTime(currentTime);

	size_t currentIndex = this->GetCurrentCommandIndex();
	size_t scriptSize = m_CachedScript.size();

	if (currentIndex >= scriptSize)
	{
		this->HandleEndOfScript();
		return;
	}

	DirectorCommand& command = m_CachedScript[currentIndex];
	if (currentTime >= command.Timestamp)
	{
		if (command.Type == CommandType::Cut)
		{
			uint8_t playerIndex = (uint8_t)replayModule.FollowedPlayerIndex;
			if (command.TargetPlayerIdx != playerIndex)
			{
				float deadline = (currentIndex + 1 < scriptSize)
					? m_CachedScript[currentIndex + 1].Timestamp
					: currentTime + 5.0f;

				g_pSystem->Debug->Log("[DirectorSystem] INFO: Execute Cut to %d", command.TargetPlayerIdx);
				this->GoToPlayer(command.TargetPlayerIdx, deadline);

				auto postNavModule = g_pState->Domain->Theater->GetModuleSnapshot();
				uint8_t postNavIndex = (uint8_t)postNavModule.FollowedPlayerIndex;
				if (postNavIndex != command.TargetPlayerIdx)
				{
					g_pSystem->Debug->Log("[DirectorSystem] WARNING: Cut failed (at %d, expected %d), retrying next Update().",
						postNavIndex, command.TargetPlayerIdx);
					return;
				}
			}
			this->IncrementCurrentCommandIndex();
		}
		else if (command.Type == CommandType::SetSpeed)
		{
			g_pSystem->Domain->Theater->SetReplaySpeed(command.SpeedValue);
			g_pSystem->Debug->Log("[DirectorSystem] INFO: Speed -> %.2fx", command.SpeedValue);
			this->IncrementCurrentCommandIndex();
		}
	}
}


float DirectorSystem::GetLastReplayTime() const { return m_LastReplayTime.load(); }
void DirectorSystem::SetLastReplayTime(float lastReplayTime) { m_LastReplayTime.store(lastReplayTime); }

size_t DirectorSystem::GetCurrentCommandIndex() const { return m_CurrentCommandIndex.load(); }
void DirectorSystem::SetCurrentCommandIndex(size_t cmdIndex) { m_CurrentCommandIndex.store(cmdIndex); }


void DirectorSystem::Cleanup()
{
	m_CachedScript = {};
	m_ScriptCached = false;

	m_CurrentCommandIndex.store(0);
	m_LastReplayTime.store(0.0f);
	m_StopDelayStartTime.store(0.0f);

	m_LastInputTime = {};

	g_pState->Domain->Director->Cleanup();

	g_pSystem->Debug->Log("[DirectorSystem] INFO: Cleanup completed.");
}


void DirectorSystem::HandleEndOfScript()
{
	bool isRecording = g_pState->Infrastructure->FFmpeg->IsRecording();
	bool stopOnLastEvent = g_pState->Infrastructure->FFmpeg->StopOnLastEvent();

	if (isRecording && stopOnLastEvent)
	{
		float stopDelayDuration = g_pState->Infrastructure->FFmpeg->GetStopDelayDuration();

		auto now = std::chrono::steady_clock::now();

		float currentRealTime = std::chrono::duration<float>(now.time_since_epoch()).count();

		if (m_StopDelayStartTime == 0.0f)
		{
			m_StopDelayStartTime = currentRealTime;
			g_pSystem->Debug->Log("[DirectorSystem] INFO: Script finished. Waiting %.2fs buffer (Real Time).", stopDelayDuration);
		}

		if (currentRealTime - m_StopDelayStartTime >= stopDelayDuration)
		{
			g_pSystem->Debug->Log("[DirectorSystem] WARNING: Final delay reached. Stopping FFmpeg.");

			g_pThread->Capture->StopRecording();

			m_StopDelayStartTime = 0.0f;
		}
	}
}


// Re-orders the timeline by timestamp (ascending), using event weight 
// as a tie-breaker for simultaneous events.
void DirectorSystem::PrioritizeEvents(std::vector<GameEvent> timeline)
{
	std::sort(timeline.begin(), timeline.end(), [](const GameEvent& a, const GameEvent& b) {
		if (abs(a.Timestamp - b.Timestamp) < 0.001f)
		{
			int weightA = g_pState->Domain->EventRegistry->GetEventWeight(a);
			int weightB = g_pState->Domain->EventRegistry->GetEventWeight(b);
			return weightA > weightB;
		}
	
		return a.Timestamp < b.Timestamp;
	});

	g_pState->Domain->Timeline->SetTimeline(timeline);
}

void DirectorSystem::RemoveDuplicates(std::vector<GameEvent> timeline)
{
	std::vector<GameEvent> uniqueEvents;

	for (const auto& currentEvent : timeline)
	{
		bool isDuplicate = false;

		for (auto i = std::ssize(uniqueEvents) - 1; i >= 0; --i)
		{
			const auto& existingEvent = uniqueEvents[i];

			float timeDiff = std::abs(currentEvent.Timestamp - existingEvent.Timestamp);

			if (timeDiff <= 1.0f)
			{
				if (currentEvent.Type == existingEvent.Type)
				{
					if (HasTheSamePlayers(currentEvent.Players, existingEvent.Players))
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

bool DirectorSystem::HasTheSamePlayers(const std::vector<PlayerInfo>& listA, const std::vector<PlayerInfo>& listB)
{
	if (listA.size() != listB.size()) return false;

	for (size_t i = 0; i < listA.size(); ++i)
	{
		if (listA[i].Id != listB[i].Id) return false;
	}

	return true;
}


void DirectorSystem::GenerateScript(std::vector<GameEvent> timeline)
{
	std::vector<ActionSegment> rawSegments;

	for (size_t i = 0; i < timeline.size(); ++i)
	{
		const auto& currentEvent = timeline[i];

		// Filter: Warmup time & empty players.
		if (currentEvent.Timestamp <= m_DirectorWarmupTime ||
			currentEvent.Players.empty()) continue;

		// Filter: Zero weight.
		int weight = g_pState->Domain->EventRegistry->GetEventWeight(currentEvent);
		if (weight == 0) continue;

		// Players[0] is the perpetrator of the event.
		const PlayerInfo* targetPlayer = &currentEvent.Players[0];
		ActionSegment segment;

		segment.PlayerName = targetPlayer->Name;
		segment.PlayerID = targetPlayer->Id;
		segment.StartTime = currentEvent.Timestamp - m_EventPadding;
		segment.EndTime = currentEvent.Timestamp + m_EventPadding;
		segment.Score = weight;
		segment.Events.push_back(currentEvent.Type);

		rawSegments.push_back(segment);
	}

	if (rawSegments.empty())
	{
		g_pSystem->Debug->Log("[DirectorSystem] WARNING: Raw segments are empty, script generation cancelled.");
		return;
	}

	RefineActionSegments(rawSegments);

	std::vector<DirectorCommand> tempScript;
	float lastSegmentEnd = m_DirectorWarmupTime;

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
		cutCmd.Timestamp = segment.StartTime - m_CameraTransitionTime;
		cutCmd.Type = CommandType::Cut;
		cutCmd.TargetPlayerIdx = segment.PlayerID;
		cutCmd.TargetPlayerName = segment.PlayerName;

		cutCmd.Reason = "Score: " + std::to_string(segment.Score) +
			" Events: " + std::to_string(segment.Events.size());

		lastSegmentEnd = segment.EndTime;

		tempScript.push_back(cutCmd);
	}

	std::sort(tempScript.begin(), tempScript.end(), [](const DirectorCommand& a, const DirectorCommand& b) {
		return a.Timestamp < b.Timestamp;
	});

	g_pState->Domain->Director->SetScript(tempScript);
	g_pUI->Director->ResetCachedScript();
	g_pSystem->Debug->Log("[DirectorSystem] INFO: Script generated.");
}

void DirectorSystem::RefineActionSegments(std::vector<ActionSegment>& segments)
{
	std::sort(segments.begin(), segments.end(), 
		[](const ActionSegment& a, const ActionSegment& b) {
		return a.StartTime < b.StartTime;
	});

	std::vector<ActionSegment> merged;
	merged.push_back(segments[0]);

	for (size_t i = 1; i < segments.size(); i++)
	{
		ActionSegment& current = segments[i];
		ActionSegment& previous = merged.back();

		float originalStartTime = current.StartTime;
		float eventTimestamp = originalStartTime + m_EventPadding;

		bool overlaps = current.StartTime <= (previous.EndTime + m_CameraTransitionTime);
		if (overlaps)
		{
			if (previous.PlayerName == current.PlayerName)
			{
				previous.EndTime = (std::max)(previous.EndTime, current.EndTime);
				previous.Score += current.Score;

				previous.Events.insert(previous.Events.end(),
					current.Events.begin(), current.Events.end());
			}
			else
			{
				float previousDurationSoFar = current.StartTime - previous.StartTime;

				float scoreMultiplier = (previous.Score > 50.0f) ? 
					2.5f : (previous.Score > 30.0f ? 1.8f : 1.3f);

				if (previousDurationSoFar > 3.0f && 
					current.Score > (previous.Score * scoreMultiplier))
				{
					previous.EndTime = current.StartTime - m_CameraTransitionTime;
					merged.push_back(current);
				}
				else
				{
					float proposedNewStartTime = previous.EndTime + m_CameraTransitionTime;
					float remainingPrePoll = eventTimestamp - proposedNewStartTime;

					if (remainingPrePoll >= (m_EventPadding / 2.0f))
					{
						current.StartTime = proposedNewStartTime;
						merged.push_back(current);
					}
				}
			}
		}
		else
		{
			float currentGap = current.StartTime - previous.EndTime;
			if (currentGap < m_CameraTransitionTime)
			{
				current.StartTime = previous.EndTime + m_CameraTransitionTime;
			}

			merged.push_back(current);
		}
	}

	std::vector<ActionSegment> finalSegments;
	for (const auto& seg : merged)
	{
		if ((seg.EndTime - seg.StartTime) >= 3.0f)
		{
			finalSegments.push_back(seg);
		}
	}

	segments = finalSegments;
}


void DirectorSystem::ForceSpectatorModes(ReplayModule replayModule)
{
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastInputTime).count();
	if (elapsed < 200) return;

	bool inputSent = false;

	CameraMode desiredCamera = g_pState->Domain->Theater->GetCameraMode();
	POVMode desiredPOV = g_pState->Domain->Theater->GetPOVMode();
	UIMode desiredUI = g_pState->Domain->Theater->GetUIMode();

	// Force camera mode.
	if (desiredCamera != CameraMode::Unselected)
	{
		CameraMode activeCamera = static_cast<CameraMode>(replayModule.CameraMode);
		if (activeCamera != desiredCamera)
		{
			g_pState->Infrastructure->Input->EnqueueRequest({
				InputContext::Theater, InputAction::ToggleCameraMode }, true);
			
			inputSent = true;
		}
	}

	// Force ui mode.
	if (!inputSent && desiredUI != UIMode::Unselected)
	{
		UIMode activeUI = static_cast<UIMode>(replayModule.UIMode);
		if (activeUI != desiredUI)
		{
			g_pState->Infrastructure->Input->EnqueueRequest({
				InputContext::Theater, InputAction::ToggleUIMode }, true);

			inputSent = true;
		}
	}

	// Force POV mode.
	if (!inputSent && desiredPOV != POVMode::Unselected)
	{
		POVMode activePOV = static_cast<POVMode>(replayModule.POVMode);
		if (activePOV != desiredPOV)
		{
			if (activePOV == POVMode::FirstPerson || activePOV == POVMode::ThirdPersonAttached)
			{
				g_pState->Infrastructure->Input->EnqueueRequest({
					InputContext::Theater, InputAction::TogglePOVMode }, true);
			}
			else
			{
				g_pState->Infrastructure->Input->EnqueueRequest({
					InputContext::Theater, InputAction::ToggleCameraMode }, true);
			}

			inputSent = true;
		}
	}

	if (inputSent)
	{
		m_LastInputTime = now;
	}
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


void DirectorSystem::GoToPlayer(uint8_t targetIdx, float nextCommandTimestamp)
{
	if (targetIdx >= 16) return;

	g_pSystem->Debug->Log("[DirectorSystem] INFO: Navigating to: %d", targetIdx);

	auto wallDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

	int maxAttempts = 32;
	for (int attempts = 0; attempts < maxAttempts; attempts++)
	{
		auto currentModule = g_pState->Domain->Theater->GetModuleSnapshot();
		uint8_t currentIndex = (uint8_t)currentModule.FollowedPlayerIndex;
		if (currentIndex == targetIdx) break;

		if (std::chrono::steady_clock::now() >= wallDeadline)
		{
			g_pSystem->Debug->Log("[DirectorSystem] WARNING: Navigation aborted, wall time limit reached.");
			return;
		}

		int forwardSteps = (targetIdx - currentIndex + 16) % 16;
		int backwardSteps = 16 - forwardSteps;
		bool movingForward = forwardSteps <= backwardSteps;
		int stepsNeeded = movingForward ? forwardSteps : backwardSteps;
		InputAction action = movingForward ? InputAction::NextPlayer : InputAction::PreviousPlayer;

		int inputsToSend = (stepsNeeded > 2) ? 2 : 1;
		for (int i = 0; i < inputsToSend; i++)
		{
			InputRequest request{};
			request.Context = InputContext::Theater;
			request.Action = action;
			g_pState->Infrastructure->Input->EnqueueRequest(request, true);
		}

		while (g_pState->Infrastructure->Input->IsProcessing())
			std::this_thread::yield();

		auto waitStart = std::chrono::steady_clock::now();
		while (std::chrono::steady_clock::now() - waitStart < std::chrono::milliseconds(200))
		{
			auto newModule = g_pState->Domain->Theater->GetModuleSnapshot();
			if ((uint8_t)newModule.FollowedPlayerIndex != currentIndex) break;
			std::this_thread::yield();
		}
	}

	auto finalModule = g_pState->Domain->Theater->GetModuleSnapshot();
	if ((uint8_t)finalModule.FollowedPlayerIndex == targetIdx)
		g_pSystem->Debug->Log("[DirectorSystem] INFO: Navigation successful.");
	else
		g_pSystem->Debug->Log("[DirectorSystem] WARNING: Navigation failed.");
}


void DirectorSystem::IncrementCurrentCommandIndex()
{
	size_t scriptSize = g_pState->Domain->Director->GetScriptSize();
	size_t current = m_CurrentCommandIndex.load();

	if (current < scriptSize)
	{
		m_CurrentCommandIndex.store(current + 1);
	}
}