#pragma once

#include "External/minhook/include/MinHook.h"
#include <atomic>
#include <string>

enum LibrarianPhase
{
    Start,
    BuildTimeline,
    ExecuteDirector,
    End
};

extern LibrarianPhase g_CurrentPhase;
extern uintptr_t g_BaseModuleAddress;
extern std::atomic<bool> g_Running;
extern std::string g_BaseDirectory;
extern HMODULE g_HandleModule;
extern DWORD g_GamePID;

namespace Main
{
    inline HMODULE GetHaloReachModuleBaseAddress() {
        return GetModuleHandle(L"haloreach.dll");
    }

    inline bool IsGameFocused()
    {
        HWND foregroundWnd = GetForegroundWindow();
        if (foregroundWnd == NULL) return false;

        DWORD foregroundPID = 0;
        GetWindowThreadProcessId(foregroundWnd, &foregroundPID);

        return (foregroundPID == g_GamePID);
    }
}