#include "pch.h"
#include "LogThread.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Utils/ThreadUtils.h"
#include "Core/Common/AppCore.h"
#include "Core/Hooks/Lifecycle/GameEngineStart_Hook.h"
#include <sstream>
#include <chrono>

using namespace std::chrono_literals;

std::thread g_LogThread;

void LogThread::Run()
{
    Logger::LogAppend("=== Log Thread Started ===");

    while (g_pState->Lifecycle.IsRunning())
    {
        if (!g_pState->Timeline.IsLoggingActive() || !g_pState->Theater.IsTheaterMode()) 
        {
            ThreadUtils::WaitOrExit(100ms);
            continue;
        }

        std::vector<GameEvent> eventsToProcess = g_pSystem->Timeline.ConsumePendingEvents();

        for (const auto& event : eventsToProcess)
        {
            std::stringstream ss;
            ss << "[" << Formatting::ToTimestamp(event.Timestamp) << "] ";
            ss << Formatting::EventTypeToString(event.Type);

            if (!event.Players.empty()) 
            {
                // TODO: Maybe do a function to format player on 'Formatting' namespace.
                ss << ": ";
                for (size_t i = 0; i < event.Players.size(); ++i) 
                {
                    ss << event.Players[i].Name;
                    if (i < event.Players.size() - 1) 
                    {
                        ss << ", ";
                    }
                }
            }

            Logger::LogAppend(ss.str().c_str());
        }

        ThreadUtils::WaitOrExit(50ms);
    }

    Logger::LogAppend("=== Log Thread Stopped ===");
}