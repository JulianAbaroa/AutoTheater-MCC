#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/DXUtils.h"
#include "Core/Common/GlobalState.h"
#include "Hooks/UserInterface/ResizeBuffers_Hook.h"
#include "External/minhook/include/MinHook.h"

ResizeBuffers_t original_ResizeBuffers = nullptr;
std::atomic<bool> g_ResizeBuffers_Hook_Installed = false;
void* g_ResizeBuffers_Address = nullptr;

HRESULT __stdcall hkResizeBuffers(
	IDXGISwapChain* pSwapChain,
	UINT BufferCount,
	UINT Width, UINT Height,
	DXGI_FORMAT NewFormat,
	UINT SwapChainFlags
) {
	if (g_pState->pMainRenderTargetView)
	{
		g_pState->pContext->OMSetRenderTargets(0, nullptr, nullptr);
		g_pState->pMainRenderTargetView->Release();
		g_pState->pMainRenderTargetView = nullptr;
	}

	HRESULT hr = original_ResizeBuffers(
		pSwapChain,
		BufferCount,
		Width, Height,
		NewFormat,
		SwapChainFlags
	);

	ID3D11Texture2D* pBackBuffer = nullptr;
	if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)))
	{
		g_pState->pDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pState->pMainRenderTargetView);
		pBackBuffer->Release();
	}

	return hr;
}

void ResizeBuffers_Hook::Install()
{
	if (g_ResizeBuffers_Hook_Installed.load()) return;

	auto addresses = DXUtils::GetVtableAddresses();
	if (!addresses.ResizeBuffers) return;

	g_ResizeBuffers_Address = addresses.ResizeBuffers;

	if (MH_CreateHook(g_ResizeBuffers_Address, &hkResizeBuffers, reinterpret_cast<LPVOID*>(&original_ResizeBuffers)))
	{
		Logger::LogAppend("Failed to create ResizeBuffers hook");
		return;
	}

	if (MH_EnableHook(g_ResizeBuffers_Address) != MH_OK) {
		Logger::LogAppend("Failed to enable ResizeBuffers hook");
		return;
	}

	g_ResizeBuffers_Hook_Installed.store(true);
	Logger::LogAppend("ResizeBuffers hook installed successfully");
}

void ResizeBuffers_Hook::Uninstall()
{
	if (!g_ResizeBuffers_Hook_Installed.load()) return;

	MH_DisableHook(g_ResizeBuffers_Address);
	MH_RemoveHook(g_ResizeBuffers_Address);

	g_ResizeBuffers_Hook_Installed.store(false);
	Logger::LogAppend("ResizeBuffers hook uninstalled");
}