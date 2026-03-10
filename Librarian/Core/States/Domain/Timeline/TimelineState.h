#pragma once

#include "Core/Common/Types/TimelineTypes.h"
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

struct TimelineState
{
public:
	std::string GetAssociatedReplayHash() const;
	void SetAssociatedReplayHash(const std::string& hash);

	// Timeline-related
	void AddGameEvent(const GameEvent& event);
	void SetTimeline(std::vector<GameEvent> newTimeline);
	std::vector<GameEvent> GetTimelineCopy() const;
	size_t GetTimelineSize() const;
	void ClearTimeline();

private:
	// Accumulates the events of a replay during the Timeline phase.
	std::vector<GameEvent> m_Events{};
	std::string m_AssociatedReplayHash{};
	mutable std::mutex m_Mutex;
	std::atomic<bool> m_IsLoggingActive{ false };
};