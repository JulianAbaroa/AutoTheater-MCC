#include "pch.h"
#include "LogThread.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Utils/ThreadUtils.h"
#include "Core/Common/GlobalState.h"
#include "Hooks/Lifecycle/GameEngineStart_Hook.h"
#include <sstream>
#include <chrono>

using namespace std::chrono_literals;

std::thread g_LogThread;

void LogThread::Run()
{
    Logger::LogAppend("=== Log Thread Started ===");

    while (g_pState->running.load())
    {
        if (!g_pState->logGameEvents.load() || !g_pState->isTheaterMode.load()) {
            ThreadUtils::WaitOrExit(100ms);
            continue;
        }

        std::vector<GameEvent> eventsToProcess;

        {
            std::lock_guard<std::mutex> lock(g_pState->timelineMutex);

            size_t currentSize = g_pState->timeline.size();
            size_t lastProcessed = g_pState->processedCount.load();

            if (lastProcessed < currentSize)
            {
                eventsToProcess.assign(g_pState->timeline.begin() + lastProcessed, g_pState->timeline.end());
                g_pState->processedCount.store(currentSize);
            }
        }

        for (const auto& gameEvent : eventsToProcess)
        {
            std::stringstream ss;
            ss << "[" << Formatting::ToTimestamp(gameEvent.Timestamp) << "] ";
            ss << Formatting::EventTypeToString(gameEvent.Type);

            if (!gameEvent.Players.empty()) {
                ss << ": ";
                for (size_t i = 0; i < gameEvent.Players.size(); ++i) {
                    ss << gameEvent.Players[i].Name;
                    if (i < gameEvent.Players.size() - 1) {
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