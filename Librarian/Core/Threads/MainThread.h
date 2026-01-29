#pragma once

#include <thread>

extern std::thread g_MainThread;

namespace MainThread
{
	void Run();
	void ShutdownAndEject();
	void UpdateToPhase(AutoTheaterPhase targetPhase);
}