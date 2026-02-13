#include "pch.h"
#include "Utils/Logger.h"
#include "LifecycleSystem.h"
#include "Core/Common/AppCore.h"

void LifecycleSystem::SignalShutdown()
{
	g_pState->Lifecycle.SetRunning(false);
	
	{
		std::lock_guard<std::mutex>	lock(g_pState->Lifecycle.GetMutex());
		g_pState->Lifecycle.GetCV().notify_all();
	}

	Logger::LogAppend("LifecycleSystem: Shutdown signaled to all threads.");
}