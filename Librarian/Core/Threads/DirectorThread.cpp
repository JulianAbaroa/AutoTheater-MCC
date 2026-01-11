#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Core/Systems/Director.h"
#include "Core/Threads/DirectorThread.h"
#include "Hooks/Lifecycle/DestroySubsystems_Hook.h"
#include <chrono>

using namespace std::chrono_literals;
std::thread g_DirectorThread;

void DirectorThread::Run()
{
    Logger::LogAppend("=== Director Thread Started ===");

    while (g_Running.load())
    {
        if (g_DirectorInitialized && !g_GameEngineDestroyed)
        {
            if (Main::IsGameFocused())
            {
                Director::Update();
            }
        }

        std::this_thread::sleep_for(16ms);
    }

    Logger::LogAppend("=== Director Thread Stopped ===");
}