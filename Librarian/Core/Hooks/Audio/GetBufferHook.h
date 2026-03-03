#pragma once

#include <Audioclient.h>

class GetBufferHook
{
public:
	void Install();
	void Uninstall();

private:
	static HRESULT __stdcall HookedGetBuffer(IAudioRenderClient* pThis, UINT32 NumFramesRequested, BYTE** ppData);
	
	typedef HRESULT(__stdcall* GetBuffer_t)(IAudioRenderClient*, UINT32, BYTE**);

	static inline GetBuffer_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};