#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/ThreadUtils.h"
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
    
    while (g_pState->Running.load())
    {
        if (g_pState->CurrentPhase.load() == AutoTheaterPhase::Director)
        {
            if (!g_pState->DirectorInitialized.load())
            {
                if (g_pState->DirectorHooksReady.load())
                {
                    Logger::LogAppend("DirectorThread: Phase match and Engine ready. Initializing...");
                    Director::Initialize();
                }
            }
            else
            {
                if (g_pState->EngineStatus.load() != EngineStatus::Destroyed  && g_pState->IsTheaterMode.load())
                {
                    Director::Update();
                }
            }

            ThreadUtils::WaitOrExit(16ms);
        }
        else {
            g_pState->DirectorHooksReady.store(false);
        }
    }

    Logger::LogAppend("=== Director Thread Stopped ===");
}