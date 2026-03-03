#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Hooks/Audio/GetServiceHook.h"
#include "External/minhook/include/MinHook.h"

// This hook intercepts service requests to map previously registered IAudioClient instances 
// to their corresponding IAudioRenderClient, enabling data capture at the rendering level.
HRESULT __stdcall GetServiceHook::HookedGetService(IAudioClient* pThis, REFIID riid, void** ppv)
{
	HRESULT hr = m_OriginalFunction(pThis, riid, ppv);
	if (!g_pState->Theater.IsTheaterMode()) return hr;

	if (SUCCEEDED(hr) && riid == __uuidof(IAudioRenderClient) && ppv != nullptr)
	{
		IAudioRenderClient* pRenderClient = (IAudioRenderClient*)*ppv;
		AudioFormat format = g_pState->Audio.GetActiveInstance(pThis);

		if (format.Channels == 8)
		{
			g_pState->Audio.RegisterActiveInstance(
				pRenderClient,
				format.Channels,
				format.SamplesPerSec,
				format.BytesPerFrame);

			g_pUtil->Log.Append("[GetService] INFO: Instance converted.");
		}
	}

	return hr;
}

void GetServiceHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = g_pSystem->Audio.GetAudioClientVTableAddress(14);
	if (!functionAddress)
	{
		g_pUtil->Log.Append("[GetService] ERROR: Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedGetService, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pUtil->Log.Append("[GetService] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pUtil->Log.Append("[GetService] ERROR: Failed to enable the hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	g_pUtil->Log.Append("[GetService] INFO: Hook installed.");
}

void GetServiceHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pUtil->Log.Append("[GetService] INFO: Hook uninstalled.");
}