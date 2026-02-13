#include "pch.h"
#include "Core/States/Interface/DebugState.h"

void DebugState::PushBack(std::string message)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Logs.push_back(std::move(message));
}

void DebugState::TrimToSize(int size)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Logs.size() > size)
	{
		m_Logs.pop_front();
	}
}

void DebugState::ForEachLog(std::function<void(const std::string&)> callback) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	for (const auto& log : m_Logs)
	{
		callback(log);
	}
}

void DebugState::ClearLogs()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Logs.clear();
}