#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Domain/Director/DirectorState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Audio/CoreAudioHook.h"
#include "Core/Hooks/Audio/AudioVTableResolver.h"
#include "Core/Hooks/Audio/GetBufferHook.h"
#include "External/minhook/include/MinHook.h"

// This hook intercepts the acquisition of the audio render buffer, allowing AutoTheater 
// to capture raw PCM data directly from the engine's audio stream.
HRESULT __stdcall GetBufferHook::HookedGetBuffer(IAudioRenderClient* pThis, UINT32 NumFramesRequested, BYTE** ppData)
{
    HRESULT hr = m_OriginalFunction(pThis, NumFramesRequested, ppData);
    if (!g_pState->Domain->Theater->IsTheaterMode()) return hr;

    if (hr == S_OK && ppData != nullptr)
    {
        g_pState->Infrastructure->Audio->SetBufferForInstance(pThis, *ppData);
    }

    return hr;
}

void GetBufferHook::Install()
{
    if (m_IsHookInstalled.load()) return;

    void* functionAddress = g_pHook->Audio->Resolver->GetRenderClientAddress(3);
    if (!functionAddress)
    {
        g_pSystem->Debug->Log("[GetBuffer] ERROR: Failed to obtain the function address.");
        return;
    }

    m_FunctionAddress.store(functionAddress);
    if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedGetBuffer, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
    {
        g_pSystem->Debug->Log("[GetBuffer] ERROR: Failed to create the hook.");
        return;
    }
    if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
    {
        g_pSystem->Debug->Log("[GetBuffer] ERROR: Failed to enable the hook.");
        return;
    }

    m_IsHookInstalled.store(true);
    g_pSystem->Debug->Log("[GetBuffer] INFO: Hook installed.");
}

void GetBufferHook::Uninstall()
{
    if (!m_IsHookInstalled.load()) return;

    MH_DisableHook(m_FunctionAddress.load());
    MH_RemoveHook(m_FunctionAddress.load());

    m_IsHookInstalled.store(false);
    g_pSystem->Debug->Log("[GetBuffer] INFO: Hook uninstalled.");
}