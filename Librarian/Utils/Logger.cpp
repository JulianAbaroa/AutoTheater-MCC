/** 
 * @file Logger.cpp
 * @brief Thread-safe logging utility for file persistence and in-game debug console.
 * * This utility synchronizes access to the log file and the global state's
 * debug buffer, ensuring messages from multiple threads (Main, Director, Input, Log)
 * are recorded without race conditions.
 */

#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/GlobalState.h"
#include <sstream>
#include <fstream>
#include <iomanip>

/** 
 * @brief Appends a formatted message to the log file and the internal debug buffer.
 * @param format The message string to be recorded.
 * * @note This method uses std::lock_guard	to protect the configuration path
 * and the vector buffer.
 */
void Logger::LogAppend(const char* format) {
	std::string loggerPathCopy;

	// 1. Thread-safe retrieval of the current log file path.
	{
		std::lock_guard<std::mutex> lock(g_pState->configMutex);
		loggerPathCopy = g_pState->loggerPath;
	}

	// 2. File Persistence Logic
	std::ofstream ofs(loggerPathCopy, std::ios::app);

	// Capture local system time for the log entry
	SYSTEMTIME system_time;
	GetLocalTime(&system_time);

	ofs << "[" << std::setw(4) << system_time.wYear << "-"
		<< std::setfill('0') << std::setw(2) << system_time.wMonth << "-"
		<< std::setw(2) << system_time.wDay << " "
		<< std::setw(2) << system_time.wHour << ":"
		<< std::setw(2) << system_time.wMinute << ":"
		<< std::setw(2) << system_time.wSecond << "] "
		<< format << "\r\n";

	ofs.close();

	// 3. UI Buffer Logic
	// Stores the last 500 entries for the ImGui debug window.
	{
		std::lock_guard lock(g_pState->logMutex);

		g_pState->debugLogs.push_back(std::string(format));

		if (g_pState->debugLogs.size() > 500)
		{
			g_pState->debugLogs.erase(g_pState->debugLogs.begin());
		}
	}
}