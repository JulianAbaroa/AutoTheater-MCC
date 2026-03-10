#include "pch.h"
#include "Core/States/Infrastructure/Persistence/ReplayState.h"

bool ReplayState::ShouldRefreshReplayList() const
{ 
	return m_ShouldRefreshReplayList.load(); 
}

std::vector<SavedReplay> ReplayState::GetSavedReplaysCacheCopy() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_SavedReplaysCache;
}


void ReplayState::SetRefreshReplayList(bool value)
{
	m_ShouldRefreshReplayList.store(value);
}

void ReplayState::SetSavedReplaysCache(std::vector<SavedReplay> cache)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_SavedReplaysCache = std::move(cache);
}