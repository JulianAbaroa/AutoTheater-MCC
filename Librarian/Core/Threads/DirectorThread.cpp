#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Core/Systems/Director.h"
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
    
    while (g_Running.load())
    {
        if (g_CurrentPhase == LibrarianPhase::ExecuteDirector)
        {
            if (!g_DirectorInitialized)
            {
                if (g_EngineHooksReady)
                {
                    Logger::LogAppend("DirectorThread: Phase match and Engine ready. Initializing...");
                    Director::Initialize();
                }
            }
            else
            {
                if (!g_GameEngineDestroyed && g_IsTheaterMode)
                {
                    Director::Update();
                }
            }

            std::this_thread::sleep_for(16ms);
        }
        else if (g_CurrentPhase == LibrarianPhase::BuildTimeline)
        {
            g_EngineHooksReady.store(false);
        }
    }

    Logger::LogAppend("=== Director Thread Stopped ===");
}