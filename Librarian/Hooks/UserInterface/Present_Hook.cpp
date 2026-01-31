#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/DXUtils.h"
#include "Core/Systems/Theater.h"
#include "Core/Common/GlobalState.h"
#include "Core/UserInterface/UserInterface.h"
#include "Hooks/UserInterface/Present_Hook.h"
#include "Hooks/UserInterface/WndProc_Hook.h"
#include "Hooks/UserInterface/GetRawInputData_Hook.h"
#include "External/minhook/include/MinHook.h"
#include "External/imgui/backends/imgui_impl_win32.h"
#include "External/imgui/backends/imgui_impl_dx11.h"
#include "External/imgui/imgui.h"
#include <sstream>
#pragma comment(lib, "d3d11.lib")

Present_t original_Present = nullptr;
std::atomic<bool> g_Present_Hook_Installed = false;
void* g_Present_Address = nullptr;

static void ApplyCustomStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 5.0f;
	style.FrameRounding = 3.0f;
	style.FramePadding = ImVec2(8, 6);
	style.WindowPadding = ImVec2(12, 12);
	style.ItemSpacing = ImVec2(10, 8);
	style.ScrollbarSize = 15.0f;

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	colors[ImGuiCol_Tab] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	colors[ImGuiCol_TabActive] = ImVec4(0.2f, 0.4f, 0.6f, 1.0f);
}

static HRESULT __stdcall hkPresent(
	IDXGISwapChain* pSwapChain, 
	UINT SyncInterval, 
	UINT Flags
) {
	static bool init = false;

	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pState->pDevice)))
		{
			g_pState->pDevice->GetImmediateContext(&g_pState->pContext);

			ID3D11Texture2D* pBackBuffer = nullptr;
			if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)))
			{
				g_pState->pDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pState->pMainRenderTargetView);
				pBackBuffer->Release();
			}

			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			g_pState->GameHWND = sd.OutputWindow;

			ImGui::CreateContext();

			ApplyCustomStyle();

			ImGuiIO& io = ImGui::GetIO();

			float width = (float)sd.BufferDesc.Width;
			float baseFontSize = 22.0f;

			if (width >= 2560) baseFontSize = 30.0f;
			if (width >= 3840) baseFontSize = 38.0f;

			ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", baseFontSize);
			if (!font)
			{
				Logger::LogAppend("WARNING: Segoeui font not found, using arial font");
				font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", baseFontSize);
			}

			g_pState->ForceMenuReset.store(true);

			ImGui_ImplWin32_Init(g_pState->GameHWND);
			ImGui_ImplDX11_Init(g_pState->pDevice, g_pState->pContext);

			original_WndProc = (WNDPROC)SetWindowLongPtr(g_State.GameHWND, GWLP_WNDPROC, (LONG_PTR)WndProc_Hook::hkWndProc);

			init = true;
			Logger::LogAppend("ImGui Initialized inside hkPresent");
		}
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	
	ImGui::NewFrame();

	if (g_pState->ShowMenu.load() && g_pState->FreezeMouse.load())
	{
		RECT rect;
		GetWindowRect(g_pState->GameHWND, &rect);
		ClipCursor(&rect);
	}
	else
	{
		ClipCursor(NULL);
	}

	ImGui::GetIO().MouseDrawCursor = g_pState->ShowMenu.load();

	UserInterface::DrawMainInterface();

	ImGui::Render();

	if (g_pState->pMainRenderTargetView)
	{
		ID3D11RenderTargetView* oldRTV = nullptr;
		ID3D11DepthStencilView* oldDSV = nullptr;
		g_pState->pContext->OMGetRenderTargets(1, &oldRTV, &oldDSV);

		g_pState->pContext->OMSetRenderTargets(1, &g_pState->pMainRenderTargetView, NULL);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pState->pContext->OMSetRenderTargets(1, &oldRTV, oldDSV);
		if (oldRTV) oldRTV->Release();
		if (oldDSV) oldDSV->Release();
	}

	if (g_pState->IsTheaterMode.load())
	{
		Theater::UpdateRealTimeScale();
	}

	return original_Present(pSwapChain, SyncInterval, Flags);
}

void Present_Hook::Install() {
    if (g_Present_Hook_Installed.load()) return;

    auto addresses = DXUtils::GetVtableAddresses();
    if (!addresses.Present) {
        Logger::LogAppend("Failed to obtain the address of Present()");
        return;
    }

    g_Present_Address = addresses.Present;
    if (MH_CreateHook(g_Present_Address, &hkPresent, reinterpret_cast<LPVOID*>(&original_Present)) != MH_OK) {
        Logger::LogAppend("Failed to create Present hook");
        return;
    }

    if (MH_EnableHook(g_Present_Address) != MH_OK) {
        Logger::LogAppend("Failed to enable Present hook");
        return;
    }

	GetRawInputData_Hook::Install();

    g_Present_Hook_Installed.store(true);
    Logger::LogAppend("Present hook installed successfully");
}

void Present_Hook::Uninstall() {
    if (!g_Present_Hook_Installed.load()) return;

	if (g_pState)
	{
		if (g_pState->GameHWND && original_WndProc)
		{
			SetWindowLongPtr(g_pState->GameHWND, GWLP_WNDPROC, (LONG_PTR)original_WndProc);
			original_WndProc = nullptr;
		}

		if (g_pState->pMainRenderTargetView)
		{
			g_pState->pMainRenderTargetView->Release();
			g_pState->pMainRenderTargetView = nullptr;
		}
	}

	if (ImGui::GetCurrentContext())
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

    MH_DisableHook(g_Present_Address);
    MH_RemoveHook(g_Present_Address);

	GetRawInputData_Hook::Uninstall();

    g_Present_Hook_Installed.store(false);
    Logger::LogAppend("Present hook uninstalled");
}