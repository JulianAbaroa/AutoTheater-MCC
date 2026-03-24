#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/AudioSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Hooks/Audio/ReleaseBufferHook.h"
#include "External/minhook/include/MinHook.h"

// This hook intercepts the buffer release signal, indicating that Blam! has finished 
// writing audio data, allowing AutoTheater to commit these samples to the recording stream.
HRESULT __stdcall ReleaseBufferHook::HookedReleaseBuffer(IAudioRenderClient* pThis, UINT32 NumFramesWritten, DWORD Flags)
{
    if (!g_pState->Domain->Theater->IsTheaterMode())
    {
        return m_OriginalFunction(pThis, NumFramesWritten, Flags);
    }
    BYTE* pBuffer = g_pState->Infrastructure->Audio->GetBufferForInstance(pThis);
    if (NumFramesWritten > 0 && pBuffer != nullptr)
    {
        AudioFormat format = g_pState->Infrastructure->Audio->GetAudioInstance(pThis);
        if (format.BytesPerFrame > 0 && (format.Channels == 8 || format.Channels == 2 || format.Channels == 6))
        {
            bool isSilent = (Flags & AUDCLNT_BUFFERFLAGS_SILENT);
            size_t dataSize = (size_t)NumFramesWritten * format.BytesPerFrame;
            g_pSystem->Infrastructure->Audio->WriteAudio(pThis, pBuffer, dataSize, isSilent);
        }
    }
    return m_OriginalFunction(pThis, NumFramesWritten, Flags);
}

void ReleaseBufferHook::Install()
{
    if (m_IsHookInstalled.load()) return;

    void* functionAddress = g_pSystem->Infrastructure->Audio->GetRenderClientVTableAddress(4);
    if (!functionAddress)
    {
        g_pSystem->Debug->Log("[ReleaseBuffer] ERROR: Failed to obtain the function address.");
        return;
    }

    m_FunctionAddress.store(functionAddress);
    if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedReleaseBuffer, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
    {
        g_pSystem->Debug->Log("[ReleaseBuffer] ERROR: Failed to create the hook.");
        return;
    }
    if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
    {
        g_pSystem->Debug->Log("[ReleaseBuffer] ERROR: Failed to enable the hook.");
        return;
    }

    m_IsHookInstalled.store(true);
    g_pSystem->Debug->Log("[ReleaseBuffer] INFO: Hook installed.");
}

void ReleaseBufferHook::Uninstall()
{
    if (!m_IsHookInstalled.load()) return;

    MH_DisableHook(m_FunctionAddress.load());
    MH_RemoveHook(m_FunctionAddress.load());

    m_IsHookInstalled.store(false);
    g_pSystem->Debug->Log("[ReleaseBuffer] INFO: Hook uninstalled.");
}