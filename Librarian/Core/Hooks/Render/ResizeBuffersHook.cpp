#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Infrastructure/Persistence/GallerySystem.h"
#include "Core/Systems/Infrastructure/Engine/RenderSystem.h"
#include "Core/Hooks/Render/ResizeBuffersHook.h"
#include "External/minhook/include/MinHook.h"

// Manages swap chain resizing to prevent crashes, updating render targets 
// and ensuring video capture is safely halted during resolution changes.
HRESULT __stdcall ResizeBuffersHook::HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
	UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	g_pState->Infrastructure->Render->SetResizing(true);

	if (g_pState->Infrastructure->FFmpeg->IsRecording())
	{
		g_pUtil->Log.Append("[ResizeBuffers] WARNING: Capture stopped due to resolution change.");
		g_pSystem->Infrastructure->FFmpeg->Stop();
		g_pSystem->Infrastructure->Gallery->RefreshList(g_pState->Infrastructure->FFmpeg->GetOutputPath());
	}

	UINT evenWidth = Width & ~1;
	UINT evenHeight = Height & ~1;
	g_pState->Infrastructure->Render->SetWidth(evenWidth);
	g_pState->Infrastructure->Render->SetHeight(evenHeight);

	if (g_pState->Infrastructure->Render->GetContext())
	{
		g_pState->Infrastructure->Render->GetContext()->OMSetRenderTargets(0, nullptr, nullptr);
	}

	g_pState->Infrastructure->Render->CleanupRTV();

	HRESULT hr = m_OriginalFunction(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	if (SUCCEEDED(hr)) g_pSystem->Infrastructure->Render->Initialize(pSwapChain);

	g_pState->Infrastructure->Render->SetResizing(false);

	return hr;
}

void ResizeBuffersHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	auto addresses = g_pUtil->DX.GetVtableAddresses();
	if (!addresses.ResizeBuffers) return;

	m_FunctionAddress.store(addresses.ResizeBuffers);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedResizeBuffers, reinterpret_cast<LPVOID*>(&m_OriginalFunction)))
	{
		g_pUtil->Log.Append("[ResizeBuffers] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		g_pUtil->Log.Append("[ResizeBuffers] ERROR: Failed to enable the hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	g_pUtil->Log.Append("[ResizeBuffers] INFO: Hook installed.");
}

void ResizeBuffersHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pUtil->Log.Append("[ResizeBuffers] INFO: Hook uninstalled.");
}