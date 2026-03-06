#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Threads/InputThread.h"
#include <chrono>

using namespace std::chrono_literals;

void InputThread::Run()
{
    g_pUtil->Log.Append("[InputThread] INFO: Started.");

    while (g_pState->Lifecycle.IsRunning())
    {
        if (g_pState->Theater.IsTheaterMode()) 
        {
            if (g_pState->Settings.ShouldUseManualInput())
            {
                g_pSystem->Input.ManualInput();
            }

            g_pSystem->Input.AutomaticInput();
        }

        g_pUtil->Thread.WaitOrExit(100ms);
    }

    g_pUtil->Log.Append("[InputThread] INFO: Stopped.");
}