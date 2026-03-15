#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/InputSystem.h"
#include "Core/Systems/Infrastructure/Engine/ThreadSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Threads/Infrastructure/InputThread.h"
#include <chrono>

using namespace std::chrono_literals;

void InputThread::Run()
{
    g_pSystem->Debug->Log("[InputThread] INFO: Started.");

    while (g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        if (g_pState->Domain->Theater->IsTheaterMode()) 
        {
            if (g_pState->Infrastructure->Settings->ShouldUseManualInput())
            {
                g_pSystem->Infrastructure->Input->ManualInput();
            }

            g_pSystem->Infrastructure->Input->AutomaticInput();
        }

        g_pSystem->Infrastructure->Thread->WaitOrExit(100ms);
    }

    g_pSystem->Debug->Log("[InputThread] INFO: Stopped.");
}