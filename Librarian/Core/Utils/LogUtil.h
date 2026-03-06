#pragma once

#include "Core/Common/Types/LogTypes.h"
#include <string>
#include <atomic>
#include <chrono>
#include <mutex>

class LogUtil
{
public:
	void Append(const char* format, ...);

    bool HasUnreadError() const;
    bool HasUnreadWarning() const;
    void ClearUnreadStates();

    std::chrono::steady_clock::time_point GetLastAlertTime() const;

private:
    std::string GetTimestampString();
    void ParseEntryTags(LogEntry& entry, std::string& body);
    void ParseLogLevel(LogEntry& entre, std::string& body);
    void UpdateAlertState(LogLevel level);
    void WriteToLogFile(const char* header, const char* message);

    std::atomic<bool> m_UnreadError{ false };
    std::atomic<bool> m_UnreadWarning{ false };
    std::chrono::steady_clock::time_point m_LastAlertTime{};
	std::mutex m_Mutex;
};