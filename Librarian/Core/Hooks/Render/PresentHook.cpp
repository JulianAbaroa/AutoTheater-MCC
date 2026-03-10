#include "pch.h"
#include "Core/UI/CoreUI.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Input/CoreInputHook.h"
#include "Core/Hooks/Input/GetRawInputDataHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/RenderSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Theater/TheaterSystem.h"
#include "Core/Hooks/Render/PresentHook.h"
#include "External/minhook/include/MinHook.h"

// Intercepts the DXGI Present call to handle frame updates, UI rendering, 
// and frame capture logic before the buffer is displayed on screen.
HRESULT __stdcall PresentHook::HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (g_pState->Infrastructure->Render->IsResizing())
    {
        return m_OriginalPresent(pSwapChain, SyncInterval, Flags);
    }

    if (!g_pSystem->Infrastructure->Render->IsInitialized())
    {
        g_pSystem->Infrastructure->Render->Initialize(pSwapChain);
    }

    // Rebuild ImGui UI
    if (g_pState->Infrastructure->Render->ShouldRebuildFonts())
    {
        g_pSystem->Infrastructure->Render->UpdateUIScale();
    }

    // Calculate Framerate 
    g_pSystem->Infrastructure->Render->UpdateFramerate();

    // Record UI disabled.
    if (!g_pState->Infrastructure->FFmpeg->ShouldRecordUI())
    {
        g_pSystem->Infrastructure->Render->TickCapture(pSwapChain);
    }

    // Draw ImGui
    if (g_pState->Infrastructure->Render->GetRTV())
    {
        g_pSystem->Infrastructure->Render->BeginFrame(pSwapChain);
        g_pUI->Main.Draw();
        g_pSystem->Infrastructure->Render->EndFrame();
    }

    // Record UI enabled.
    if (g_pState->Infrastructure->FFmpeg->ShouldRecordUI())
    {
        g_pSystem->Infrastructure->Render->TickCapture(pSwapChain);
    }

    if (g_pState->Domain->Theater->IsTheaterMode())
    {
        g_pSystem->Domain->Theater->UpdateRealTimeScale();
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

	g_pHook->Input->GetRawInputData->Install();

    m_PresentHookInstalled.store(true);
    g_pUtil->Log.Append("[Present] INFO: Hook installed.");
}

void PresentHook::Uninstall() 
{
    if (!m_PresentHookInstalled.load()) return;

    g_pSystem->Infrastructure->Render->Shutdown();

    if (m_PresentAddress)
    {
        MH_DisableHook(m_PresentAddress);
        MH_RemoveHook(m_PresentAddress);
    }

    g_pHook->Input->GetRawInputData->Uninstall();

    m_PresentHookInstalled.store(false);
    g_pUtil->Log.Append("[Present] INFO: Hook uninstalled.");
}