#pragma once

#include <d3d11.h>

#define PRESENT_VMT_INDEX 8

typedef HRESULT(__stdcall* Present_t)(
	IDXGISwapChain* pSwapChain,
	UINT SyncInterval, 
	UINT Flags
);

namespace Present_Hook
{
	void Install();
	void Uninstall();
}