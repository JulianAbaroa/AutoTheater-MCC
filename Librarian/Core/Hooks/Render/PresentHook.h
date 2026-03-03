#pragma once

#include <d3d11.h>
#include <atomic>
#pragma comment(lib, "d3d11.lib")

class PresentHook
{
public:
	void Install();
	void Uninstall();
	
private:
	static HRESULT __stdcall HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

	typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

	static inline Present_t m_OriginalPresent{ nullptr };
	std::atomic<bool> m_PresentHookInstalled{ false };
	void* m_PresentAddress{ nullptr };
};