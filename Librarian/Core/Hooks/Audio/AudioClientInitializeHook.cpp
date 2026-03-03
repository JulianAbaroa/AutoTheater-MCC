#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Hooks/Audio/AudioClientInitializeHook.h"
#include "External/minhook/include/MinHook.h"

// This hook intercepts the initialization of WASAPI AudioClient instances created by Blam! 
// to capture and register the 7.1 surround sound stream used during theater playback.
HRESULT __stdcall AudioClientInitializeHook::HookedAudioClientInitialize(
	IAudioClient* pThis,
	AUDCLNT_SHAREMODE ShareMode,
	DWORD StreamFlags,
	REFERENCE_TIME hnsBufferDuration,
	REFERENCE_TIME hnsPeriodicity,
	const WAVEFORMATEX* pFormat,
	LPCGUID AudioSessionGuid)
{
	if (pFormat != nullptr && pFormat->nChannels == 8 && g_pState->Theater.IsTheaterMode())
	{
		g_pState->Audio.RegisterActiveInstance(
			pThis, 
			pFormat->nChannels, 
			pFormat->nSamplesPerSec,
			pFormat->nBlockAlign);

		g_pUtil->Log.Append("[AudioClientInitialize] INFO: Instance registered.");
	}

	return m_OriginalFunction(pThis, ShareMode, StreamFlags, hnsBufferDuration, hnsPeriodicity, pFormat, AudioSessionGuid);
}

void AudioClientInitializeHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = g_pSystem->Audio.GetAudioClientVTableAddress(3);
	if (!functionAddress)
	{
		g_pUtil->Log.Append("[AudioClientInitialize] ERROR: Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedAudioClientInitialize, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pUtil->Log.Append("[AudioClientInitialize] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pUtil->Log.Append("[AudioClientInitialize] ERROR: Failed to enable the hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	g_pUtil->Log.Append("[AudioClientInitialize] INFO: Hook installed.");
}

void AudioClientInitializeHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pUtil->Log.Append("[AudioClientInitialize] INFO: Hook uninstalled.");
}