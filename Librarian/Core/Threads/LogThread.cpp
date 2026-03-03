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
                char playerList[256]{};
                size_t offset = 0;

                // TODO: Maybe do a function to format player on 'Format' class.
                for (size_t i = 0; i < event.Players.size(); ++i) 
                {
                    int written = snprintf(playerList + offset, sizeof(playerList) - offset,
                        "%s%s", event.Players[i].Name.c_str(), 
                        (i < event.Players.size() ? ", " : ""));

                    if (written > 0) offset += written;
                    if (offset >= sizeof(playerList)) break;
                }
                
                g_pUtil->Log.Append("[LogThread] INFO: [%s] %s: %s", timestamp.c_str(), eventType.c_str(), playerList);
            }
        }

        g_pUtil->Thread.WaitOrExit(50ms);
    }

    g_pUtil->Log.Append("[LogThread] INFO: Stopped.");
}