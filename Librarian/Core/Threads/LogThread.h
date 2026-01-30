#pragma once

#include <thread>

extern std::thread g_LogThread;

namespace LogThread
{
	void Run();
}