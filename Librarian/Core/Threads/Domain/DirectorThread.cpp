#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Director/DirectorState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Director/DirectorSystem.h"
#include "Core/Threads/Domain/DirectorThread.h"
#include <chrono>

using namespace std::chrono_literals;

void DirectorThread::Run()
{
    g_pUtil->Log.Append("[DirectorThread] INFO: Started.");
    
    while (g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        if (g_pState->Infrastructure->Lifecycle->GetCurrentPhase() == Phase::Director)
        {
            if (!g_pState->Domain->Director->IsInitialized() && 
                g_pState->Domain->Director->AreHooksReady())
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
        
            g_pUtil->Thread.WaitOrExit(16ms);
        }
        else 
        {
            g_pState->Domain->Director->SetHooksReady(false);
        }
    }

    g_pUtil->Log.Append("[DirectorThread] INFO: Stopped.");
}