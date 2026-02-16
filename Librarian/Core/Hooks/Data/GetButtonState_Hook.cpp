#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/AppCore.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include "Core/Hooks/Data/GetButtonState_Hook.h"
#include "External/minhook/include/MinHook.h"

GetButtonState_t original_GetButtonState = nullptr;
std::atomic<bool> g_GetButtonState_Hook_Installed = false;
void* g_GetButtonState_Address = nullptr;

// This function polls keyboard events at a high frequency (approximately every 3ms).
// Note: This specific handler does not process mouse events.
// Critical: This function is extremely sensitive to stack manipulation and timing.
// Avoid using blocking mechanisms (e.g., std::mutex) as they generate undefined behavior.
// Observed behavior: Blocking or desynchronizing this thread causes the engine to
// repeatedly execute the recieved action every 3ms, leading to input flooding.
// Input Data: Recieves a unique 'buttonID', which represents the engine's internal mapping for each physical key.
// Reference: See "Core/Common/Types/InputTypes.h" for the Theater-specific key-mapped struct. 
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