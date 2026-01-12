#pragma once

#include <thread>
#include <atomic>

extern std::thread g_MainThread;

namespace MainThread
{
	void Run();
}