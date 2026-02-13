#pragma once

#include <d3d11.h>

typedef HRESULT(__stdcall* ResizeBuffers_t)(
	IDXGISwapChain* pSwapChain,
	UINT BufferCount, 
	UINT Width, UINT Height,
	DXGI_FORMAT NewFormat,
	UINT SwapChainFlags
);

namespace ResizeBuffers_Hook
{
	void Install();
	void Uninstall();
}