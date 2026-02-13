#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/ThreadUtils.h"
#include "Core/Common/AppCore.h"
#include "Core/Threads/DirectorThread.h"
#include "Core/Systems/Domain/DirectorSystem.h"
#include "Core/Hooks/Lifecycle/DestroySubsystems_Hook.h"
#include "Core/Hooks/Lifecycle/EngineInitialize_Hook.h"
#include "Core/Hooks/Lifecycle/GameEngineStart_Hook.h"
#include <chrono>

using namespace std::chrono_literals;

std::thread g_DirectorThread;

void DirectorThread::Run()
{
    Logger::LogAppend("=== Director Thread Started ===");
    
    while (g_pState->Lifecycle.IsRunning())
    {
        if (g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Director)
        {
            if (!g_pState->Director.IsInitialized())
            {
                if (g_pState->Director.AreHooksReady())
                {
                    Logger::LogAppend("DirectorThread: Phase match and Engine ready. Initializing...");
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
        
            ThreadUtils::WaitOrExit(16ms);
        }
        else {
            g_pState->Director.SetHooksReady(false);
        }
    }
    
    Logger::LogAppend("=== Director Thread Stopped ===");
}