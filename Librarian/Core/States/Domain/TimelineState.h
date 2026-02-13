#pragma once

#include "Core/Common/Types/TimelineTypes.h"
#include <vector>
#include <atomic>
#include <mutex>

class TimelineState
{
public:
	bool IsLoggingActive() const;
	void SetLoggingActive(bool value);

	// Timeline-related
	void AddGameEvent(const GameEvent& event);
	void SetTimeline(std::vector<GameEvent> newTimeline);
	std::vector<GameEvent> GetTimelineCopy() const;
	size_t GetTimelineSize() const;
	void ClearTimeline();

private:
	// Accumulates the events of a replay during the Timeline phase.
	std::vector<GameEvent> m_Events{};

	// Mutex for thread-safe Timeline modification.
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_IsLoggingActive{ true };
};