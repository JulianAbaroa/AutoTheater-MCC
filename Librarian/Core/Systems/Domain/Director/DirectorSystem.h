#pragma once

#include "Core/Common/Types/DirectorTypes.h"
#include <unordered_map>
#include <vector>
#include <atomic>

class DirectorSystem
{
public:
	void Initialize();
	void Update();

	float GetLastReplayTime() const;
	void SetLastReplayTime(float lastReplayTime);

	size_t GetCurrentCommandIndex() const;
	void SetCurrentCommandIndex(size_t cmdIndex);
	void IncrementCurrentCommandIndex();

private:
	void GoToPlayer(uint8_t targetIdx, float nextCommandTimestamp);

	void GenerateScript(std::vector<GameEvent> timeline);
	void OptimizeSegments(std::vector<ActionSegment>& segments);
	int GetWeight(const GameEvent& event, const std::unordered_map<std::wstring, EventInfo>& localRegistry);

	void PrioritizeEvents(std::vector<GameEvent> timeline);
	bool ArePlayerSetsEqual(const std::vector<PlayerInfo>& listA, const std::vector<PlayerInfo>& listB);
	void RemoveDuplicates(std::vector<GameEvent> timeline);
	float SafeGetCurrentTime();

	std::atomic<size_t> m_CurrentCommandIndex{ 0 };
	std::atomic<float> m_LastReplayTime{ 0.0f };
	std::atomic<float> m_StopDelayStartTime{ 0.0f };
	
	const float m_DirectorWarmupTime = 5.0f;
};