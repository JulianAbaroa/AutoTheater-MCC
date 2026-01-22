#pragma once

#include "Hooks/Data/GetButtonState_Hook.h"
#include <thread>
#include <queue>

extern std::thread g_InputThread;

struct InputRequest
{
	InputAction Action;
	InputContext Context;
};

extern std::atomic<bool> g_InputProcessing;
extern std::queue<InputRequest> g_InputQueue;
extern std::mutex g_InputQueueMutex;

namespace InputThread
{
	void Run();
}