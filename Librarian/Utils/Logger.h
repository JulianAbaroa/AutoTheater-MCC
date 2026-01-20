#pragma once

#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>

extern std::string g_LoggerPath;

namespace Logger
{
	void LogAppend(const char* fmt);
}