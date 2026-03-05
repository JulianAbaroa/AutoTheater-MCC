#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Hooks/Input/GetButtonStateHook.h"
#include "External/minhook/include/MinHook.h"

// This function polls keyboard events at a high frequency (approximately every 3ms).
// Note: This specific handler does not process mouse events.
// Critical: This function is extremely sensitive to stack manipulation and timing.
// Avoid using blocking mechanisms (e.g., std::mutex) as they generate undefined behavior.
// Observed behavior: Blocking or desynchronizing this thread causes the engine to
// repeatedly execute the recieved action every 3ms, leading to input flooding.
// Input Data: Recieves a unique 'buttonID', which represents the engine's internal mapping for each physical key.
// Reference: See "Core/Common/Types/InputTypes.h" for the Theater-specific key-mapped struct. 
char __fastcall GetButtonStateHook::HookedGetButtonState(short buttonID)
{
    auto nextInput = g_pState->Input.GetNextRequest();
    if (nextInput.Context == InputContext::Theater && nextInput.Action != InputAction::Unknown)
    {
        if (static_cast<short>(nextInput.Action) == buttonID)
        {
            return 1;
        }
    }

    return m_OriginalFunction(buttonID);
}

void GetButtonStateHook::Install()
{
    if (m_IsHookInstalled.load()) return;

    void* functionAddress = (void*)g_pSystem->Scanner.FindPattern(Signatures::GetButtonState);
    if (!functionAddress)
    {
        g_pUtil->Log.Append("[GetButtonState] ERROR: Failed to obtain the function address.");
        return;
    }

    m_FunctionAddress.store(functionAddress);
    if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedGetButtonState, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
    {
        g_pUtil->Log.Append("[GetButtonState] ERROR: Failed to create the hook.");
        return;
    }
    if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
    {
        g_pUtil->Log.Append("[GetButtonState] ERROR: Failed to enable the hook.");
        return;
    }

    m_IsHookInstalled.store(true);
    g_pUtil->Log.Append("[GetButtonState] INFO: Hook installed.");
}

void GetButtonStateHook::Uninstall()
{
    if (!m_IsHookInstalled.load()) return;

    MH_DisableHook(m_FunctionAddress.load());
    MH_RemoveHook(m_FunctionAddress.load());

    m_IsHookInstalled.store(false);
    g_pUtil->Log.Append("[GetButtonState] INFO: Hook uninstalled.");
}