#include "pch.h"
#include "Utils/Logger.h"
#include <mutex>

std::string g_LoggerPath;
static std::mutex g_LogMutex;

void Logger::LogAppend(const char* format) {
	std::lock_guard<std::mutex> lock(g_LogMutex);
	std::ofstream ofs(g_LoggerPath, std::ios::app);

	if (!ofs.is_open()) {
		std::stringstream ss;
		ss << "logAppend: failed to open '" << g_LoggerPath << "' - msg: " << format;
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
}