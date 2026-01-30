#pragma once

#include "Core/Common/Types/AutoTheaterTypes.h"
#include <thread>

extern std::thread g_MainThread;

namespace MainThread
{
	void Run();
	void ShutdownAndEject();
	void UpdateToPhase(AutoTheaterPhase targetPhase);
}