#pragma once

#include <thread>

extern std::thread g_MainThread;

namespace MainThread
{
	void Run();
}