#include "pch.h"
#include "Core/Utils/LogUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Common/Types/LogTypes.h"
#include <fstream>

// TODO: Rework this method.
void LogUtil::Append(const char* format, ...)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    SYSTEMTIME st;
    GetLocalTime(&st);

    char header[64]{};
    int headerLen = snprintf(header, sizeof(header), "%04u-%02u-%02u %02u:%02u:%02u ",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    char message[1024]{};
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    LogEntry entry;
    entry.Timestamp = header;
    std::string currentBody = message;

    size_t tagStart = currentBody.find('[');
    size_t tagEnd = currentBody.find(']', tagStart);

    if (tagStart != std::string::npos && tagEnd != std::string::npos)
    {
        entry.Tag = currentBody.substr(tagStart, tagEnd - tagStart + 1);

        currentBody = currentBody.substr(tagEnd + 1);

        if (!currentBody.empty() && currentBody[0] == ' ')
            currentBody.erase(0, 1);
    }
    else
    {
        entry.Tag = "";
    }

    if (currentBody.find("ERROR:") == 0)
    {
        entry.Level = LogLevel::Error;
        entry.MessagePrefix = "ERROR: ";
        entry.Message = currentBody.substr(7);
    }
    else if (currentBody.find("WARNING:") == 0)
    {
        entry.Level = LogLevel::Warning;
        entry.MessagePrefix = "WARNING: ";
        entry.Message = currentBody.substr(9);
    }
    else if (currentBody.find("INFO:") == 0)
    {
        entry.Level = LogLevel::Info;
        entry.MessagePrefix = "INFO: ";
        entry.Message = currentBody.substr(6);
    }
    else
    {
        entry.Level = LogLevel::Default;
        entry.MessagePrefix = "";
        entry.Message = currentBody;
    }

    std::string tagPart = entry.Tag.empty() ? "" : entry.Tag + " ";
    entry.FullText = entry.Timestamp + tagPart + entry.MessagePrefix + entry.Message;

    g_pSystem->Debug.AddLog(entry);

    std::ofstream ofs(g_pState->Settings.GetLoggerPath(), std::ios::app);
    if (ofs.is_open())
    {
        ofs.write(header, headerLen);
        ofs.write(message, strlen(message));
        ofs.write("\n", 1);
        ofs.close();
    }
}