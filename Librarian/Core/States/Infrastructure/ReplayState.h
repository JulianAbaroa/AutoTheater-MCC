#pragma once

#include "Core/Common/Types/UITypes.h"
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
	
	std::string GetCurrentReplayPath() const;
	std::string GetPreviousReplayPath() const;
	std::string GetPreviousReplayHash() const;

	void SetRefreshReplayList(bool value);
	void SetSavedReplaysCache(std::vector<SavedReplay> cache);

	void SetCurrentReplayPath(const std::string& path);
	void SetPreviousReplayPath(const std::string& path);
	void SetPreviousReplayHash(const std::string& hash);

	void ClearActiveSession();

private:
	std::vector<SavedReplay> m_SavedReplaysCache;
	std::string m_PreviousReplayPath{};
	std::string m_ActiveReplayHash{};
	std::string m_CurrentFilmPath{};
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_ShouldRefreshReplayList{ true };
};