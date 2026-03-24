#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Director/DirectorState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Director/DirectorSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ThreadSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Threads/Domain/DirectorThread.h"
#include <chrono>

using namespace std::chrono_literals;

void DirectorThread::Run()
{
    g_pSystem->Debug->Log("[DirectorThread] INFO: Started.");
    
    while (g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        if (g_pState->Infrastructure->Lifecycle->GetCurrentPhase() == Phase::Director)
        {
            if (!g_pState->Domain->Director->IsInitialized() && 
                g_pState->Domain->Director->AreHooksReady() &&
                !g_pState->Domain->Director->IsSkipped())
            {
                g_pSystem->Domain->Director->Initialize();
            }
            else
            {
                if (g_pState->Infrastructure->Lifecycle->GetEngineStatus() != EngineStatus::Destroyed && 
                    g_pState->Domain->Theater->IsTheaterMode())
                {
                    g_pSystem->Domain->Director->Update();
                }
            }
        
            g_pSystem->Infrastructure->Thread->WaitOrExit(16ms);
        }
    }

    g_pSystem->Debug->Log("[DirectorThread] INFO: Stopped.");
}