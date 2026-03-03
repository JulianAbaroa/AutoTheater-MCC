#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/Utils/ThreadUtil.h"

bool ThreadUtil::WaitOrExit(std::chrono::milliseconds ms)
{
	std::unique_lock<std::mutex> lock(g_pState->Lifecycle.GetMutex());

	bool shutdownTriggered = g_pState->Lifecycle.GetCV().wait_for(lock, ms, [] {
		return !g_pState->Lifecycle.IsRunning();
	});

	return !shutdownTriggered;
}