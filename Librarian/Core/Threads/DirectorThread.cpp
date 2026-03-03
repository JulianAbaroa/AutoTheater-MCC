#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Threads/DirectorThread.h"
#include <chrono>

using namespace std::chrono_literals;

void DirectorThread::Run()
{
    g_pUtil->Log.Append("[DirectorThread] INFO: Started.");
    
    while (g_pState->Lifecycle.IsRunning())
    {
        if (g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Director)
        {
            if (!g_pState->Director.IsInitialized())
            {
                if (g_pState->Director.AreHooksReady())
                {
                    g_pUtil->Log.Append("[DirectorThread] INFO: Phase match and Engine ready, initializing.");
                    g_pSystem->Director.Initialize();
                }
            }
            else
            {
                if (g_pState->Lifecycle.GetEngineStatus() != EngineStatus::Destroyed && g_pState->Theater.IsTheaterMode())
                {
                    g_pSystem->Director.Update();
                }
            }
        
            g_pUtil->Thread.WaitOrExit(16ms);
        }
        else 
        {
            g_pState->Director.SetHooksReady(false);
        }
    }

    g_pUtil->Log.Append("[DirectorThread] INFO: Stopped.");
}