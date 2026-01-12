#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Core/Systems/Timeline.h"
#include "Core/Threads/InputThread.h"
#include <map>

std::thread g_InputThread;

void InputThread::Run()
{
    Logger::LogAppend("=== Input Thread started ===");

    static const std::map<int, float> speedMap = {
        {'0', 16.0f},  {'9', 8.0f},   {'8', 4.0f},
        {'7', 1.0f},   {'6', 0.5f},   {'5', 0.25f},
        {'4', 0.1f},   {'3', 0.0f},
    };

    while (g_Running.load())
    {
        for (const auto& [key, speed] : speedMap)
        {
            if (GetAsyncKeyState(key) & 0x8000)
            {
                Theater::SetReplaySpeed(speed);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    Logger::LogAppend("=== Input Thread Stopped ===");
}