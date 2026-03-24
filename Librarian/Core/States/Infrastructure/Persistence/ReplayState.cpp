#include "pch.h"
#include "Core/States/Infrastructure/Persistence/ReplayState.h"

bool ReplayState::ShouldRefreshReplayList() const { return m_ShouldRefreshReplayList.load(); }
void ReplayState::SetRefreshReplayList(bool value) { m_ShouldRefreshReplayList.store(value); }

std::vector<SavedReplay> ReplayState::GetSavedReplaysCacheCopy() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_SavedReplaysCache;
}

void ReplayState::SetSavedReplaysCache(std::vector<SavedReplay> cache)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_SavedReplaysCache = std::move(cache);
}