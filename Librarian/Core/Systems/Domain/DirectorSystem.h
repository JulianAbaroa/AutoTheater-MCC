#pragma once

class DirectorSystem
{
public:
	void Initialize();
	void Update();

	size_t GetCurrentCommandIndex() const;
	void SetCurrentCommandIndex(size_t cmdIndex);
	void IncrementCurrentCommandIndex();

	float GetLastReplayTime() const;
	void SetLastReplayTime(float lastReplayTime);

private:
	const float DIRECTOR_WARMUP_TIME = 5.0f;

	std::atomic<size_t> m_CurrentCommandIndex{ 0 };
	std::atomic<float> m_LastReplayTime{ 0.0f };

	void GoToPlayer(uint8_t targetIdx, float nextCommandTimestamp);

	void GenerateScript(std::vector<GameEvent> timeline);
	void OptimizeSegments(std::vector<ActionSegment>& segments);
	int GetWeight(const GameEvent& event, const std::unordered_map<std::wstring, EventInfo>& localRegistry);

	void PrioritizeEvents(std::vector<GameEvent> timeline);
	bool ArePlayerSetsEqual(const std::vector<PlayerInfo>& listA, const std::vector<PlayerInfo>& listB);
	void RemoveDuplicates(std::vector<GameEvent> timeline);
	float SafeGetCurrentTime();
};