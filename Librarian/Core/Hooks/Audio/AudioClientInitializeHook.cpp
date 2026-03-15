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
	if (pFormat != nullptr && pFormat->nChannels == 8 && g_pState->Domain->Theater->IsTheaterMode())
	{
		g_pState->Infrastructure->Audio->RegisterAudioInstance(
			pThis, 
			pFormat->nChannels, 
			pFormat->nSamplesPerSec,
			pFormat->nBlockAlign);

		g_pSystem->Debug->Log("[AudioClientInitialize] INFO: Audio instance registered.");
	}
	
	// g_pSystem->Debug->Log("[AudioClientInitialize] INFO: Instance: %p | Channels: %d", pThis, pFormat->nChannels);

	return m_OriginalFunction(pThis, ShareMode, StreamFlags, hnsBufferDuration, hnsPeriodicity, pFormat, AudioSessionGuid);
}

void AudioClientInitializeHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = g_pSystem->Infrastructure->Audio->GetAudioClientVTableAddress(3);
	if (!functionAddress)
	{
		g_pSystem->Debug->Log("[AudioClientInitialize] ERROR: Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedAudioClientInitialize, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pSystem->Debug->Log("[AudioClientInitialize] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pSystem->Debug->Log("[AudioClientInitialize] ERROR: Failed to enable the hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[AudioClientInitialize] INFO: Hook installed.");
}

void AudioClientInitializeHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[AudioClientInitialize] INFO: Hook uninstalled.");
}