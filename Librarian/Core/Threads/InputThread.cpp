#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/ThreadUtils.h"
#include "Core/Common/AppCore.h"
#include "Core/Threads/InputThread.h"
#include <chrono>

using namespace std::chrono_literals;

std::thread g_InputThread;

void InputThread::Run()
{
    Logger::LogAppend("=== Input Thread started ===");

    while (g_pState->Lifecycle.IsRunning())
    {
        if (g_pState->Theater.IsTheaterMode()) {
            g_pSystem->Input.ManualInput();
            g_pSystem->Input.AutomaticInput();
        }

        ThreadUtils::WaitOrExit(100ms);
    }

    Logger::LogAppend("=== Input Thread Stopped ===");
}