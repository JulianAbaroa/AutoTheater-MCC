#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Threads/LogThread.h"
#include <chrono>

using namespace std::chrono_literals;

void LogThread::Run()
{
    g_pUtil->Log.Append("[LogThread] INFO: Started.");

    while (g_pState->Lifecycle.IsRunning())
    {
        if (!g_pState->Timeline.IsLoggingActive() || !g_pState->Theater.IsTheaterMode()) 
        {
            g_pUtil->Thread.WaitOrExit(100ms);
            continue;
        }

        std::vector<GameEvent> eventsToProcess = g_pSystem->Timeline.ConsumePendingEvents();

        for (const auto& event : eventsToProcess)
        {
            std::string timestamp = g_pUtil->Format.ToTimestamp(event.Timestamp);
            std::string eventType = g_pUtil->Format.EventTypeToString(event.Type);

            if (!event.Players.empty())
            {
                std::string playerList;
                for (size_t i = 0; i < event.Players.size(); ++i)
                {
                    playerList += event.Players[i].Name;

                    if (i < event.Players.size() - 1)
                    {
                        playerList += ", ";
                    }
                }

                g_pUtil->Log.Append("[LogThread] INFO: [%s] %s: %s",
                    timestamp.c_str(),
                    eventType.c_str(),
                    playerList.c_str());
            }
        }

        g_pUtil->Thread.WaitOrExit(50ms);
    }

    g_pUtil->Log.Append("[LogThread] INFO: Stopped.");
}