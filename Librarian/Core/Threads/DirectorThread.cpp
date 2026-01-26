#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Systems/Director.h"
#include "Core/Common/GlobalState.h"
#include "Core/Threads/DirectorThread.h"
#include "Hooks/Lifecycle/DestroySubsystems_Hook.h"
#include "Hooks/Lifecycle/EngineInitialize_Hook.h"
#include "Hooks/Lifecycle/GameEngineStart_Hook.h"
#include <chrono>

using namespace std::chrono_literals;

std::thread g_DirectorThread;

void DirectorThread::Run()
{
    Logger::LogAppend("=== Director Thread Started ===");
    
    while (g_pState->running.load())
    {
        if (g_pState->currentPhase.load() == Phase::ExecuteDirector)
        {
            if (!g_pState->directorInitialized.load())
            {
                if (g_pState->directorHooksReady.load())
                {
                    Logger::LogAppend("DirectorThread: Phase match and Engine ready. Initializing...");
                    Director::Initialize();
                }
            }
            else
            {
                if (g_pState->engineStatus.load() != EngineStatus::Destroyed  && g_pState->isTheaterMode.load())
                {
                    Director::Update();
                }
            }

            std::this_thread::sleep_for(16ms);
        }
        else if (g_pState->currentPhase.load() == Phase::BuildTimeline) {
            g_pState->directorHooksReady.store(false);
        }
    }

    Logger::LogAppend("=== Director Thread Stopped ===");
}