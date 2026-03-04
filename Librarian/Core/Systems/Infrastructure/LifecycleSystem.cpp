#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"

void LifecycleSystem::SignalShutdown()
{
	g_pState->Lifecycle.SetRunning(false);

	{
		std::lock_guard<std::mutex>	lock(g_pState->Lifecycle.GetMutex());
		g_pState->Lifecycle.GetCV().notify_all();
	}

	g_pUtil->Log.Append("[LifecycleSystem] WARNING: Shutdown signaled to all threads.");
}