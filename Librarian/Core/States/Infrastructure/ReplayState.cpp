#include "pch.h"
#include "Core/States/Infrastructure/ReplayState.h"

bool ReplayState::ShouldRefreshReplayList() const
{ 
	return m_ShouldRefreshReplayList.load(); 
}

std::vector<SavedReplay> ReplayState::GetSavedReplaysCacheCopy() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_SavedReplaysCache;
}

std::string ReplayState::GetPreviousReplayPath() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_PreviousReplayPath;
}

std::string ReplayState::GetPreviousReplayHash() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_ActiveReplayHash;
}

std::string ReplayState::GetCurrentReplayPath() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_CurrentFilmPath;
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

void ReplayState::SetPreviousReplayHash(const std::string& hash)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_ActiveReplayHash = hash;
}

void ReplayState::SetCurrentReplayPath(const std::string& path)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_CurrentFilmPath = path;
}

void ReplayState::SetPreviousReplayPath(const std::string& path)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_PreviousReplayPath = path;
}

void ReplayState::ClearActiveSession()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_PreviousReplayPath = "";
	m_ActiveReplayHash = "";
	m_CurrentFilmPath = "";
}