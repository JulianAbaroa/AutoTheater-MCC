#pragma once

#include <Audioclient.h>
#include <atomic>

class ReleaseBufferHook
{
public:
	void Install();
	void Uninstall();

private:
	static HRESULT __stdcall HookedReleaseBuffer(IAudioRenderClient* pThis, UINT32 NumFramesWritten, DWORD Flags);
	
	typedef HRESULT(__stdcall* ReleaseBuffer_t)(IAudioRenderClient*, UINT32, DWORD);

	static inline ReleaseBuffer_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};