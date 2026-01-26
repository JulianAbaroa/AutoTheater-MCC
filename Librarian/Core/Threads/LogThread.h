#pragma once

#include "Core/Common/Types.h"
#include <thread>
#include <string>

extern std::thread g_LogThread;

namespace LogThread
{
	void Run();
}