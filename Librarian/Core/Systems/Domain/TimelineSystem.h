#pragma once

#include "Core/Common/Types/BlamTypes.h"
#include <string>
#include <deque>

class TimelineSystem
{
public:
	void ProcessEngineEvent(float timestamp, std::wstring& templateStr, EventData* rawData);

	// Extracts and returns events that have not yet been sent to the log.
	std::vector<GameEvent> ConsumePendingEvents();

	// Returns the highest Timestamp among all the events captured.
	float GetLatestTimestamp() const;

	bool HasReachedLastEvent() const;
	void SetLastEventReached(bool value);

	size_t GetLoggedEventsCount() const;
	void SetLoggedEventsCount(size_t value);

private:
	std::atomic<bool> m_LastEventReached{ false };
	std::atomic<size_t> m_LoggedEventsCount{ 0 };

	std::deque<GameEvent> m_DeduplicationHistory;
	const size_t MAX_HISTORY = 10;

	std::vector<PlayerInfo> ExtractPlayers(EventData* rawData);
	bool IsDuplicate(const GameEvent& newEvent);
};