#pragma once

#include "Core/Common/Types/UserInterfaceTypes.h"
#include <functional>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

struct ReplayState
{
public:
	bool ShouldRefreshReplayList() const;
	std::vector<SavedReplay> GetSavedReplaysCacheCopy() const;
	FilmMetadata GetFilmMetadata() const;
	std::string GetActiveReplayHash() const;
	std::string GetCurrentFilmPath() const;
	std::string GetPreviousReplayPath() const;

	void SetRefreshReplayList(bool value);
	void SetSavedReplaysCache(std::vector<SavedReplay> cache);
	void SetFilmMetadata(FilmMetadata metadata);
	void SetActiveReplayHash(const std::string& hash);
	void SetCurrentFilmPath(const std::string& path);
	void SetPreviousReplayPath(const std::string& path);

private:
	std::vector<SavedReplay> m_SavedReplaysCache;
	std::string m_PreviousReplayPath{};
	std::string m_ActiveReplayHash{};
	std::string m_CurrentFilmPath{};
	FilmMetadata m_FilmMetadata{};
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_ShouldRefreshReplayList{ true };
};