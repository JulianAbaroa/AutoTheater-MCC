#pragma once

#include "Core/Common/Types/DirectorTypes.h"
#include <unordered_map>
#include <vector>
#include <atomic>
#include <chrono>

class DirectorSystem
{
public:
	void Initialize();
	void Update();

	float GetLastReplayTime() const;
	void SetLastReplayTime(float lastReplayTime);

	size_t GetCurrentCommandIndex() const;
	void SetCurrentCommandIndex(size_t cmdIndex);

	void Cleanup();

private:
	void HandleEndOfScript();

	void PrioritizeEvents(std::vector<GameEvent> timeline);
	void RemoveDuplicates(std::vector<GameEvent> timeline);
	bool HasTheSamePlayers(const std::vector<PlayerInfo>& listA, const std::vector<PlayerInfo>& listB);

	void GenerateScript(std::vector<GameEvent> timeline);
	void RefineActionSegments(std::vector<ActionSegment>& segments);
	
	void ForceSpectatorModes(ReplayModule replayModule);
	float SafeGetCurrentTime();

	void GoToPlayer(uint8_t targetIdx, float nextCommandTimestamp);

	void IncrementCurrentCommandIndex();

	std::vector<DirectorCommand> m_CachedScript{};
	bool m_ScriptCached = false;

	std::atomic<size_t> m_CurrentCommandIndex{ 0 };
	std::atomic<float> m_LastReplayTime{ 0.0f };
	std::atomic<float> m_StopDelayStartTime{ 0.0f };

	std::chrono::steady_clock::time_point m_LastInputTime;
	
	const float m_CameraTransitionTime = 3.0f;
	const float m_DirectorWarmupTime = 5.0f;
	const float m_LookAheadWindow = 8.0f;
	const float m_EventPadding = 5.0f;

};