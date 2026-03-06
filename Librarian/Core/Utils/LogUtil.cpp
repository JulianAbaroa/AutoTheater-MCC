#include "pch.h"
#include "Core/Utils/LogUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Common/Types/LogTypes.h"
#include <fstream>

void LogUtil::Append(const char* format, ...)
{
    char messageBuffer[1024]{};
    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    va_end(args);

    std::lock_guard<std::mutex> lock(m_Mutex);

    LogEntry entry;
    entry.Timestamp = this->GetTimestampString();
    std::string currentBody = messageBuffer;

    this->ParseEntryTags(entry, currentBody);
    this->ParseLogLevel(entry, currentBody);

    this->UpdateAlertState(entry.Level);

    std::string tagPart = entry.Tag.empty() ? "" : entry.Tag + " ";
    entry.FullText = entry.Timestamp + tagPart + entry.MessagePrefix + entry.Message;

    g_pSystem->Debug.AddLog(entry);
    this->WriteToLogFile(entry.Timestamp.c_str(), messageBuffer);
}


std::string LogUtil::GetTimestampString()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[64]{};
    snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u ",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::string(buf);
}

void LogUtil::ParseEntryTags(LogEntry& entry, std::string& body)
{
    size_t tagStart = body.find('[');
    size_t tagEnd = body.find(']', tagStart);

    if (tagStart != std::string::npos && tagEnd != std::string::npos)
    {
        entry.Tag = body.substr(tagStart, tagEnd - tagStart + 1);
        body = body.substr(tagEnd + 1);

        if (!body.empty() && body[0] == ' ') body.erase(0, 1);
    }
    else
    {
        entry.Tag = "";
    }
}

void LogUtil::ParseLogLevel(LogEntry& entry, std::string& body)
{
    if (body.find("ERROR:") == 0) 
    {
        entry.Level = LogLevel::Error;
        entry.MessagePrefix = "ERROR: ";
        entry.Message = body.substr(7);
    }
    else if (body.find("WARNING:") == 0) 
    {
        entry.Level = LogLevel::Warning;
        entry.MessagePrefix = "WARNING: ";
        entry.Message = body.substr(9);
    }
    else if (body.find("INFO:") == 0) 
    {
        entry.Level = LogLevel::Info;
        entry.MessagePrefix = "INFO: ";
        entry.Message = body.substr(6);
    }
    else 
    {
        entry.Level = LogLevel::Default;
        entry.MessagePrefix = "";
        entry.Message = body;
    }
}

void LogUtil::UpdateAlertState(LogLevel level)
{
    if (level == LogLevel::Error || level == LogLevel::Warning)
    {
        if (level == LogLevel::Error) m_UnreadError.store(true);
        else m_UnreadWarning.store(true);

        m_LastAlertTime = std::chrono::steady_clock::now();
    }
}

void LogUtil::WriteToLogFile(const char* header, const char* message)
{
    std::ofstream ofs(g_pState->Settings.GetLoggerPath(), std::ios::app);
    if (ofs.is_open())
    {
        ofs << header << message << "\n";
        ofs.close();
    }
}


std::chrono::steady_clock::time_point LogUtil::GetLastAlertTime() const { return m_LastAlertTime; }
bool LogUtil::HasUnreadError() const { return m_UnreadError.load(); }
bool LogUtil::HasUnreadWarning() const { return m_UnreadWarning.load(); }

void LogUtil::ClearUnreadStates() {
    m_UnreadError.store(false);
    m_UnreadWarning.store(false);
}
