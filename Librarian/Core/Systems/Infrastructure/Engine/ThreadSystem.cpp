#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/Systems/Infrastructure/Engine/ThreadSystem.h"

bool ThreadSystem::WaitOrExit(std::chrono::milliseconds ms)
{
	std::unique_lock<std::mutex> lock(g_pState->Infrastructure->Lifecycle->GetMutex());

	bool shutdownTriggered = g_pState->Infrastructure->Lifecycle->GetCV().wait_for(lock, ms, [] {
		return !g_pState->Infrastructure->Lifecycle->IsRunning();
	});

	return !shutdownTriggered;
}