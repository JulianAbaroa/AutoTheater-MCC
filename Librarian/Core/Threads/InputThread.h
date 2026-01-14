#pragma once

#include <thread>

extern std::thread g_InputThread;

namespace InputThread
{
	void Run();
}