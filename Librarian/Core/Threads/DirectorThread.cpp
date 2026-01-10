#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Core/Systems/Director.h"
#include "Core/Threads/DirectorThread.h"
#include <chrono>

using namespace std::chrono_literals;
std::thread g_DirectorThread;

void DirectorThread::Run()
{
    Logger::LogAppend("=== Director Thread Started ===");

    while (g_Running.load())
    {
        if (g_DirectorInitialized)
        {
            Director::Update();
        }

        std::this_thread::sleep_for(16ms);
    }

    Logger::LogAppend("=== Director Thread Stopped ===");
}