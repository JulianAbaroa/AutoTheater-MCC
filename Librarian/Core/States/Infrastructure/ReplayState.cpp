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

FilmMetadata ReplayState::GetFilmMetadata() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_FilmMetadata;
}

std::string ReplayState::GetPreviousReplayPath() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_PreviousReplayPath;
}

std::string ReplayState::GetActiveReplayHash() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_ActiveReplayHash;
}

std::string ReplayState::GetCurrentFilmPath() const
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

void ReplayState::SetFilmMetadata(FilmMetadata metadata)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_FilmMetadata = std::move(metadata);
}

void ReplayState::SetActiveReplayHash(const std::string& hash)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_ActiveReplayHash = hash;
}

void ReplayState::SetCurrentFilmPath(const std::string& path)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_CurrentFilmPath = path;
}

void ReplayState::SetPreviousReplayPath(const std::string& path)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_PreviousReplayPath = path;
}