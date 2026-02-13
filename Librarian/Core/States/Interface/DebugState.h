#pragma once

#include <functional>
#include <string>
#include <deque>
#include <mutex>

struct DebugState
{
public:
	void PushBack(std::string message);
	void TrimToSize(int size);
	void ForEachLog(std::function<void(const std::string&)> callback) const;
	void ClearLogs();

private:
	std::deque<std::string> m_Logs{};
	const size_t m_MaxLogs = 500;

	// Mutex for thread-safe Logs modification.
	mutable std::mutex m_Mutex;
};