#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Core/Systems/Timeline.h"
#include "Core/Threads/InputThread.h"
#include "Hooks/Lifecycle/GameEngineStart_Hook.h"
#include "Hooks/Data/SpectatorHandleInput_Hook.h"
#include <functional>
#include <map>

std::thread g_InputThread;

std::atomic<bool> g_InputProcessing{ false };
std::queue<InputRequest> g_InputQueue;
std::mutex g_InputQueueMutex;

bool ExecuteInputWithFeedback(InputRequest req, std::function<bool()> successCondition, int timeoutMs, int stabilizeMs)
{
    g_InputProcessing.store(true);

    g_NextInput.InputContext = req.Context;
    g_NextInput.InputAction = req.Action;

    auto startWait = std::chrono::steady_clock::now();
    bool success = false;

    while (std::chrono::steady_clock::now() - startWait < std::chrono::milliseconds(timeoutMs))
    {
        if (successCondition())
        {
            success = true;
            break;
        }
        std::this_thread::yield();
    }

    g_NextInput.InputAction = InputAction::Unknown;

    std::this_thread::sleep_for(std::chrono::milliseconds(stabilizeMs));

    g_InputProcessing.store(false);
    return success;
}

void InputThread::Run()
{
    Logger::LogAppend("=== Input Thread started (State-Based Logic) ===");

    static const std::map<int, float> speedMap = {
        {'0', 16.0f},  {'9', 8.0f},   {'8', 4.0f},
        {'7', 1.0f},   {'6', 0.5f},   {'5', 0.25f},
        {'4', 0.1f},   {'3', 0.0f},
    };

    while (g_Running.load())
    {
        if (!g_IsTheaterMode) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        InputRequest currentReq = { InputAction::Unknown, InputContext::Unknown };

        {
            std::lock_guard<std::mutex> lock(g_InputQueueMutex);
            if (!g_InputQueue.empty())
            {
                currentReq = g_InputQueue.front();
                g_InputQueue.pop();
            }
        }

        if (currentReq.Action != InputAction::Unknown)
        {
            if (currentReq.Action == InputAction::NextPlayer || currentReq.Action == InputAction::PreviousPlayer)
            {
                uint8_t initialIdx = g_FollowedPlayerIdx;

                auto condition = [initialIdx]() {
                    return g_FollowedPlayerIdx != initialIdx;
                    };

                ExecuteInputWithFeedback(currentReq, condition, 500, 20);
            }
            else if (currentReq.Action == InputAction::ToggleFreecam)
            {
                uint8_t initialCamState = g_CameraAttached;

                auto condition = [initialCamState]() {
                    return g_CameraAttached != initialCamState;
                    };

                if (!ExecuteInputWithFeedback(currentReq, condition, 500, 50)) {
                    Logger::LogAppend("[InputThread] Warning: Freecam toggle timed out.");
                }
            }
            else
            {
                auto condition = []() { return false; };

                ExecuteInputWithFeedback(currentReq, condition, 50, 50);
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        for (const auto& [key, speed] : speedMap)
        {
            if (GetAsyncKeyState(key) & 0x8000)
            {
                Theater::SetReplaySpeed(speed);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                break;
            }
        }
    }

    Logger::LogAppend("=== Input Thread Stopped ===");
}