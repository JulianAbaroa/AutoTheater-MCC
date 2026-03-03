#pragma once

#include "Core/Common/Types/LogTypes.h"
#include <functional>
#include <string>
#include <atomic>
#include <deque>
#include <mutex>

struct DebugState
{
public:
	void PushBack(LogEntry entry);
	void TrimToSize(int size);

	void ForEachLog(std::function<void(const LogEntry&)> callback) const;
	void ClearLogs();

	int GetMaxCapacity() const;

	LogEntry GetLogAt(size_t index) const;
	size_t GetTotalLogs() const;

	void RemoveIf(std::function<bool(const LogEntry&)> predicate);

private:
	std::deque<LogEntry> m_Logs{};
	mutable std::mutex m_Mutex;

	const std::atomic<int> m_MaxCapacity{ 500 };
};