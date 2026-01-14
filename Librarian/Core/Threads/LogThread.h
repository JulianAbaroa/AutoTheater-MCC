#pragma once

#include "Core/Systems/Timeline.h"
#include <thread>
#include <string>

extern std::thread g_LogThread;
extern bool g_LogGameEvents;

namespace LogThread
{
	void Run();
	std::string EventTypeToString(EventType type);
}