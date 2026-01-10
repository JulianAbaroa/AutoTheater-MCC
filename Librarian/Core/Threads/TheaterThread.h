#pragma once

#include "Core/Systems/Timeline.h"
#include <thread>

extern std::thread g_TheaterThread;

namespace TheaterThread
{
	void Run();
	std::string EventTypeToString(EventType type);
}