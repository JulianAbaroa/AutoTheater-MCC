#pragma once

#include <Audioclient.h>
#include <atomic>

class AudioClientInitializeHook
{
public:
	void Install();
	void Uninstall();

private:
	static HRESULT __stdcall HookedAudioClientInitialize(IAudioClient* pThis, AUDCLNT_SHAREMODE ShareMode, DWORD StreamFlags, REFERENCE_TIME hnsBufferDuration, REFERENCE_TIME hnsPeriodicity, const WAVEFORMATEX* pFormat, LPCGUID AudioSessionGuid);

	typedef HRESULT(__stdcall* AudioClientInitialize_t)(IAudioClient*, AUDCLNT_SHAREMODE,
		DWORD, REFERENCE_TIME, REFERENCE_TIME, const WAVEFORMATEX*, LPCGUID);

	static inline AudioClientInitialize_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};