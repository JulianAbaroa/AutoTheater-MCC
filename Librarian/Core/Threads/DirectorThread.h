#pragma once

#include <thread>

extern std::thread g_DirectorThread;

namespace DirectorThread
{
	void Run();
}