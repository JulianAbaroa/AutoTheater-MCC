#pragma once

#include <windows.h>
#include <fstream>
#include <sstream>

extern std::string g_LoggerPath;

namespace Logger
{
	void LogAppend(const char* fmt);
}