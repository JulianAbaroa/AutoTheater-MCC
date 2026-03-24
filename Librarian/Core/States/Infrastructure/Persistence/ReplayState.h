#pragma once

#include "Core/Common/Types/UITypes.h"
#include <functional>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

class ReplayState
{
public:
	bool ShouldRefreshReplayList() const;
	void SetRefreshReplayList(bool value);

	std::vector<SavedReplay> GetSavedReplaysCacheCopy() const;
	void SetSavedReplaysCache(std::vector<SavedReplay> cache);

private:
	std::vector<SavedReplay> m_SavedReplaysCache;
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_ShouldRefreshReplayList{ true };
};