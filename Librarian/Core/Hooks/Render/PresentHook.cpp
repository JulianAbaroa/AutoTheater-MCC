#include "pch.h"
#include "Core/UI/CoreUI.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "External/minhook/include/MinHook.h"

// Intercepts the DXGI Present call to handle frame updates, UI rendering, 
// and frame capture logic before the buffer is displayed on screen.
HRESULT __stdcall PresentHook::HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (g_pState->Render.IsResizing())
    {
        return m_OriginalPresent(pSwapChain, SyncInterval, Flags);
    }

    if (!g_pSystem->Render.IsInitialized())
    {
        g_pSystem->Render.Initialize(pSwapChain);
    }

    // Calculate Framerate 
    g_pSystem->Render.UpdateFramerate();

    if (!g_pState->FFmpeg.ShouldRecordUI())
    {
        g_pSystem->Render.TickCapture(pSwapChain);
    }

    // Draw ImGui
    if (g_pState->Render.GetRTV()) 
    {
        g_pSystem->Render.BeginFrame(pSwapChain);
        g_pUI->Main.Draw();
        g_pSystem->Render.EndFrame();
    }

    if (g_pState->FFmpeg.ShouldRecordUI())
    {
        g_pSystem->Render.TickCapture(pSwapChain);
    }

    if (g_pState->Theater.IsTheaterMode()) 
    {
        g_pSystem->Theater.UpdateRealTimeScale();
    }

    return m_OriginalPresent(pSwapChain, SyncInterval, Flags);
}

void PresentHook::Install() 
{
    if (m_PresentHookInstalled.load()) return;

    auto addresses = g_pUtil->DX.GetVtableAddresses();
    if (!addresses.Present) 
    {
        g_pUtil->Log.Append("[Present] ERROR: Failed to obtain the function address.");
        return;
    }

    m_PresentAddress = addresses.Present;
    if (MH_CreateHook(m_PresentAddress, &this->HookedPresent, reinterpret_cast<LPVOID*>(&m_OriginalPresent)) != MH_OK) 
    {
        g_pUtil->Log.Append("[Present] ERROR: Failed to create the hook.");
        return;
    }
    if (MH_EnableHook(m_PresentAddress) != MH_OK) 
    {
        g_pUtil->Log.Append("[Present] ERROR: Failed to enable the hook.");
        return;
    }

	g_pHook->GetRawInputData.Install();

    m_PresentHookInstalled.store(true);
    g_pUtil->Log.Append("[Present] INFO: Hook installed.");
}

void PresentHook::Uninstall() 
{
    if (!m_PresentHookInstalled.load()) return;

    g_pSystem->Render.Shutdown();

    if (m_PresentAddress)
    {
        MH_DisableHook(m_PresentAddress);
        MH_RemoveHook(m_PresentAddress);
    }

    g_pHook->GetRawInputData.Uninstall();

    m_PresentHookInstalled.store(false);
    g_pUtil->Log.Append("[Present] INFO: Hook uninstalled.");
}