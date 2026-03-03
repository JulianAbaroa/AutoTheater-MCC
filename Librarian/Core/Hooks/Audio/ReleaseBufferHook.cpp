#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Hooks/Audio/ReleaseBufferHook.h"
#include "External/minhook/include/MinHook.h"

// This hook intercepts the buffer release signal, indicating that Blam! has finished 
// writing audio data, allowing AutoTheater to commit these samples to the recording stream.
HRESULT __stdcall ReleaseBufferHook::HookedReleaseBuffer(IAudioRenderClient* pThis, UINT32 NumFramesWritten, DWORD Flags)
{
    if (!g_pState->Theater.IsTheaterMode()) return m_OriginalFunction(pThis, NumFramesWritten, Flags);

    BYTE* pBuffer = g_pState->Audio.GetLastBuffer();
    g_pState->Audio.SetLastBuffer(nullptr);

    if (NumFramesWritten > 0 && pBuffer != nullptr)
    {
        bool isSilent = (Flags & AUDCLNT_BUFFERFLAGS_SILENT);
        AudioFormat format = g_pState->Audio.GetActiveInstance(pThis);
        size_t dataSize = (size_t)NumFramesWritten * format.BytesPerFrame;
        g_pSystem->Audio.WriteAudio(pThis, pBuffer, dataSize, isSilent);
        // g_pUtil->Log.LogAppend("[ReleaseBuffer] Audio writed.");
    }

    return m_OriginalFunction(pThis, NumFramesWritten, Flags);
}

void ReleaseBufferHook::Install()
{
    if (m_IsHookInstalled.load()) return;

    void* functionAddress = g_pSystem->Audio.GetRenderClientVTableAddress(4);
    if (!functionAddress)
    {
        g_pUtil->Log.Append("[ReleaseBuffer] ERROR: Failed to obtain the function address.");
        return;
    }

    m_FunctionAddress.store(functionAddress);
    if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedReleaseBuffer, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
    {
        g_pUtil->Log.Append("[ReleaseBuffer] ERROR: Failed to create the hook.");
        return;
    }
    if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
    {
        g_pUtil->Log.Append("[ReleaseBuffer] ERROR: Failed to enable the hook.");
        return;
    }

    m_IsHookInstalled.store(true);
    g_pUtil->Log.Append("[ReleaseBuffer] INFO: Hook installed.");
}

void ReleaseBufferHook::Uninstall()
{
    if (!m_IsHookInstalled.load()) return;

    MH_DisableHook(m_FunctionAddress.load());
    MH_RemoveHook(m_FunctionAddress.load());

    m_IsHookInstalled.store(false);
    g_pUtil->Log.Append("[ReleaseBuffer] INFO: Hook uninstalled.");
}