#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/AppCore.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include "Core/Hooks/Data/GetButtonState_Hook.h"
#include "External/minhook/include/MinHook.h"

GetButtonState_t original_GetButtonState = nullptr;
std::atomic<bool> g_GetButtonState_Hook_Installed = false;
void* g_GetButtonState_Address = nullptr;

char __fastcall hkGetButtonState(short buttonID)
{
    auto nextInput = g_pState->Input.GetNextRequest();
    if (nextInput.Context == InputContext::Theater && nextInput.Action != InputAction::Unknown)
    {
        if (static_cast<short>(nextInput.Action) == buttonID)
        {
            return 1;
        }
    }

    return original_GetButtonState(buttonID);
}

void GetButtonState_Hook::Install()
{
    if (g_GetButtonState_Hook_Installed.load()) return;

    void* methodAddress = (void*)Scanner::FindPattern(Signatures::GetButtonState);
    if (!methodAddress)
    {
        Logger::LogAppend("Failed to obtain the address of GetButtonState()");
        return;
    }

    g_GetButtonState_Address = methodAddress;

    if (MH_CreateHook(g_GetButtonState_Address, &hkGetButtonState, reinterpret_cast<LPVOID*>(&original_GetButtonState)) != MH_OK)
    {
        Logger::LogAppend("Failed to create GetButtonState hook");
        return;
    }

    if (MH_EnableHook(g_GetButtonState_Address) != MH_OK)
    {
        Logger::LogAppend("Failed to enable GetButtonState hook");
        return;
    }

    g_GetButtonState_Hook_Installed.store(true);
    Logger::LogAppend("GetButtonState hook installed");
}

void GetButtonState_Hook::Uninstall()
{
    if (!g_GetButtonState_Hook_Installed.load()) return;

    MH_DisableHook(g_GetButtonState_Address);
    MH_RemoveHook(g_GetButtonState_Address);

    g_GetButtonState_Hook_Installed.store(false);
    Logger::LogAppend("GetButtonState hook uninstalled");
}