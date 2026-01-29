#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/ThreadUtils.h"
#include "Core/Systems/Theater.h"
#include "Core/Common/GlobalState.h"
#include "Core/Threads/InputThread.h"
#include <functional>
#include <chrono>
#include <map>

using namespace std::chrono_literals;

std::thread g_InputThread;

static bool ExecuteInputWithFeedback(InputRequest req, std::function<bool()> successCondition, int timeoutMs, int stabilizeMs)
{
    g_pState->inputProcessing.store(true);
    g_pState->nextInput.store({req.Context, req.Action });

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

    g_pState->nextInput.store({ InputContext::Theater, InputAction::Unknown });

    std::this_thread::sleep_for(std::chrono::milliseconds(stabilizeMs));

    std::lock_guard lock(g_pState->inputMutex);
    g_pState->inputProcessing.store(false);
    return success;
}

void InputThread::Run()
{
    Logger::LogAppend("=== Input Thread started ===");

    static const std::map<int, float> speedMap = {
        {'0', 16.0f},  {'9', 8.0f},   {'8', 4.0f},
        {'7', 1.0f},   {'6', 0.5f},   {'5', 0.25f},
        {'4', 0.1f},   {'3', 0.0f},
    };

    while (g_pState->running.load())
    {
        if (!g_pState->isTheaterMode.load()) {
            ThreadUtils::WaitOrExit(100ms);
            continue;
        }

        InputRequest currentReq = { InputAction::Unknown, InputContext::Unknown };

        {
            std::lock_guard<std::mutex> lock(g_pState->inputMutex);
            if (!g_pState->inputQueue.empty())
            {
                currentReq = g_pState->inputQueue.front();
                g_pState->inputQueue.pop();
            }
        }

        if (currentReq.Action != InputAction::Unknown)
        {
            if (currentReq.Action == InputAction::NextPlayer || currentReq.Action == InputAction::PreviousPlayer)
            {
                uint8_t initialIdx = g_pState->followedPlayerIdx.load();

                auto condition = [initialIdx]() {
                    return g_pState->followedPlayerIdx.load() != initialIdx;
                };

                ExecuteInputWithFeedback(currentReq, condition, 500, 20);
            }
            else if (currentReq.Action == InputAction::ToggleFreecam)
            {
                uint8_t initialCamState = g_pState->cameraAttached.load();

                auto condition = [initialCamState]() {
                    return g_pState->cameraAttached.load() != initialCamState;
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
            ThreadUtils::WaitOrExit(5ms);
        }

        for (const auto& [key, speed] : speedMap)
        {
            if (GetAsyncKeyState(key) & 0x8000)
            {
                Theater::SetReplaySpeed(speed);
                ThreadUtils::WaitOrExit(100ms);
                break;
            }
        }
    }

    Logger::LogAppend("=== Input Thread Stopped ===");
}