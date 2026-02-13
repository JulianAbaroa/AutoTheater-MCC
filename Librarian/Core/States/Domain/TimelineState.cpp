#include "pch.h"
#include "Core/States/Domain/TimelineState.h"

bool TimelineState::IsLoggingActive() const 
{ 
	return m_IsLoggingActive.load(); 
}


void TimelineState::SetLoggingActive(bool value) 
{ 
	m_IsLoggingActive.store(value); 
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
}