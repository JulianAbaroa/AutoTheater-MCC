#pragma once

#include <d3d11.h>
#include <atomic>

class ResizeBuffersHook
{
public:
	void Install();
	void Uninstall();

private:
	static HRESULT __stdcall HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
		UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

	typedef HRESULT(__stdcall* ResizeBuffers_t)(IDXGISwapChain* pSwapChain, UINT BufferCount,
		UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

	static inline ResizeBuffers_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};