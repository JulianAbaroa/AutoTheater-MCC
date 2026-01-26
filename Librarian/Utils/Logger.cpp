#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/GlobalState.h"
#include <sstream>
#include <fstream>
#include <iomanip>

void Logger::LogAppend(const char* format) {
	std::string loggerPathCopy;

	{
		std::lock_guard<std::mutex> lock(g_pState->configMutex);
		loggerPathCopy = g_pState->loggerPath;
	}

	std::ofstream ofs(loggerPathCopy, std::ios::app);

	if (!ofs.is_open()) {
		std::stringstream ss;
		ss << "logAppend: failed to open '" << loggerPathCopy << "' - msg: " << format;
		OutputDebugStringA(ss.str().c_str());
		return;
	}

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

	{
		std::lock_guard lock(g_pState->logMutex);

		g_pState->debugLogs.push_back(std::string(format));

		if (g_pState->debugLogs.size() > 500)
		{
			g_pState->debugLogs.erase(g_pState->debugLogs.begin());
		}
	}
}