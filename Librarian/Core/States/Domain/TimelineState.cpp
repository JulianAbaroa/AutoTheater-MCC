#include "pch.h"
#include "Core/States/Domain/TimelineState.h"

std::string TimelineState::GetAssociatedReplayHash() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_AssociatedReplayHash;
}

void TimelineState::SetAssociatedReplayHash(const std::string& hash)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_AssociatedReplayHash = hash;
}


void TimelineState::AddGameEvent(const GameEvent& event)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Events.push_back(event);
}

void TimelineState::SetTimeline(std::vector<GameEvent> newTimeline)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Events = std::move(newTimeline);
}

std::vector<GameEvent> TimelineState::GetTimelineCopy() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Events;
}

size_t TimelineState::GetTimelineSize() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Events.size();
}

void TimelineState::ClearTimeline()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Events.clear();
	m_AssociatedReplayHash.clear();
}