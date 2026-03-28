#pragma once

#include "Core/Common/Types/BlamTypes.h"
#include "Core/Common/Types/TimelineTypes.h"
#include <string>
#include <vector>
#include <atomic>
#include <deque>

class TimelineSystem
{
public:
	void ProcessEngineEvent(float timestamp, std::wstring& templateStr, EventData* rawData);

	bool HasReachedLastEvent() const;
	void SetLastEventReached(bool value);

	void Cleanup();

private:
	std::vector<PlayerInfo> ExtractPlayers(EventData* rawData);
	bool IsDuplicate(const GameEvent& newEvent);

	float m_LastEventTimestamp = -1.0f;
	std::atomic<bool> m_LastEventReached{ false };
	std::deque<GameEvent> m_DeduplicationHistory{};
	const size_t m_MaxHistory = 10;
};