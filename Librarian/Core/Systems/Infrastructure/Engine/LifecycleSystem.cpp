#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/Engine/LifecycleSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

void LifecycleSystem::SignalShutdown()
{
	g_pState->Infrastructure->Lifecycle->SetRunning(false);

	{
		std::lock_guard<std::mutex>	lock(g_pState->Infrastructure->Lifecycle->GetMutex());
		g_pState->Infrastructure->Lifecycle->GetCV().notify_all();
	}

	g_pSystem->Debug->Log("[LifecycleSystem] WARNING: Shutdown signaled to all threads.");
}