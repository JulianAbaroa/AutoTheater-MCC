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

static void PressKey(WORD vKey, int delayMs = 20)
{
    if (!Main::IsGameFocused()) return;

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = MapVirtualKey(vKey, MAPVK_VK_TO_VSC);
    input.ki.dwFlags = KEYEVENTF_SCANCODE;

    if (vKey == VK_UP || vKey == VK_DOWN || vKey == VK_LEFT || vKey == VK_RIGHT) {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    SendInput(1, &input, sizeof(INPUT));
    Sleep(delayMs);

    input.ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void InputThread::ToggleHUD() { PressKey('Z'); }
void InputThread::TogglePlaybackHUD() { PressKey('X'); }

void InputThread::ToggleCameraMode() { PressKey('C'); }
void InputThread::ToggleFreecam() { PressKey(VK_SPACE); }

void InputThread::NextPlayer() { PressKey(VK_DOWN); }
void InputThread::PrevPlayer() { PressKey(VK_UP); }

void InputThread::JumpForward() { PressKey(VK_RIGHT); }
void InputThread::JumpBack() { PressKey(VK_LEFT); }

void InputThread::ResetCamera() {
    if (!Main::IsGameFocused()) return;

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;

    input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    SendInput(1, &input, sizeof(INPUT));

    Sleep(10);

    input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput(1, &input, sizeof(INPUT));
}