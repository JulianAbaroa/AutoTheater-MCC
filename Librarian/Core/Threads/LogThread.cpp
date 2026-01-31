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

    while (g_pState->Running.load())
    {
        if (!g_pState->LogGameEvents.load() || !g_pState->IsTheaterMode.load()) {
            ThreadUtils::WaitOrExit(100ms);
            continue;
        }

        std::vector<GameEvent> eventsToProcess;

        {
            std::lock_guard<std::mutex> lock(g_pState->TimelineMutex);

            size_t currentSize = g_pState->Timeline.size();
            size_t lastProcessed = g_pState->ProcessedCount.load();

            if (lastProcessed < currentSize)
            {
                eventsToProcess.assign(g_pState->Timeline.begin() + lastProcessed, g_pState->Timeline.end());
                g_pState->ProcessedCount.store(currentSize);
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