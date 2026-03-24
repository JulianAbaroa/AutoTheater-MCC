#pragma once

#include "Core/Common/Types/TimelineTypes.h"
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

class TimelineState
{
public:
	std::string GetAssociatedReplayHash() const;
	void SetAssociatedReplayHash(const std::string& hash);

	void AddGameEvent(const GameEvent& event);

	std::vector<GameEvent> GetTimelineCopy() const;
	void SetTimeline(std::vector<GameEvent> newTimeline);
	size_t GetTimelineSize() const;
	void ClearTimeline();

private:
	// Accumulates the events of a replay during the Timeline phase.
	std::vector<GameEvent> m_Events{};
	std::string m_AssociatedReplayHash{};
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_IsLoggingActive{ false };
};