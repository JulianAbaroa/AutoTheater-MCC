#pragma once

#include <Audioclient.h>
#include <atomic>

class GetServiceHook
{
public: 
	void Install();
	void Uninstall();

private:
	static HRESULT __stdcall HookedGetService(IAudioClient* pThis, REFIID riid, void** ppv);
	
	typedef HRESULT(__stdcall* AudioClientGetService_t)(IAudioClient*, REFIID, void**);

	static inline AudioClientGetService_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};
