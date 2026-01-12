#pragma once

#include "Core/Systems/Timeline.h"
#include <thread>

extern std::thread g_TheaterThread;
extern bool g_LogGameEvents;

namespace TheaterThread
{
	void Run();
	std::string EventTypeToString(EventType type);
}