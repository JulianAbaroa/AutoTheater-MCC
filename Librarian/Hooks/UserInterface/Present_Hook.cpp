#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Systems/Theater.h"
#include "Core/Common/GlobalState.h"
#include "Core/UserInterface/UserInterface.h"
#include "Hooks/UserInterface/Present_Hook.h"
#include "Hooks/UserInterface/WndProc_Hook.h"
#include "Hooks/UserInterface/GetRawInputData_Hook.h"
#include "External/minhook/include/MinHook.h"
#include "External/imgui/imgui_impl_win32.h"
#include "External/imgui/imgui_impl_dx11.h"
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
			g_pState->gameHWND = sd.OutputWindow;

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

			ImGui_ImplWin32_Init(g_pState->gameHWND);
			ImGui_ImplDX11_Init(g_pState->pDevice, g_pState->pContext);

			original_WndProc = (WNDPROC)SetWindowLongPtr(g_State.gameHWND, GWLP_WNDPROC, (LONG_PTR)WndProc_Hook::hkWndProc);

			init = true;
			Logger::LogAppend("ImGui Initialized inside hkPresent");
		}
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	
	ImGui::NewFrame();

	if (g_pState->showMenu.load() && g_pState->freezeMouse.load())
	{
		RECT rect;
		GetWindowRect(g_pState->gameHWND, &rect);
		ClipCursor(&rect);
	}
	else
	{
		ClipCursor(NULL);
	}

	ImGui::GetIO().MouseDrawCursor = g_pState->showMenu.load();

	UserInterface::DrawMainInterface();

	ImGui::Render();
	g_pState->pContext->OMSetRenderTargets(1, &g_pState->pMainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	if (g_pState->isTheaterMode)
	{
		Theater::UpdateRealTimeScale();
	}

	return original_Present(pSwapChain, SyncInterval, Flags);
}

static void* LocateDX11PresentAddress() {
	void** pVMT = nullptr;
	std::stringstream ss;

	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = DefWindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = L"DummyWindowClass";

	if (!RegisterClassEx(&wc)) {
		Logger::LogAppend("ERROR: RegisterClassEx failed");
		return nullptr;
	}

	HWND hWnd = CreateWindowEx(
		0, wc.lpszClassName, L"Dummy Window",
		WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
		NULL, NULL, wc.hInstance, NULL
	);

	if (!hWnd) {
		Logger::LogAppend("ERROR: CreateWindowEx failed");
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return nullptr;
	}

	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 1;
	sd.BufferDesc.Width = 800;
	sd.BufferDesc.Height = 600;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	ID3D11Device* pDevice = nullptr;
	IDXGISwapChain* pSwapChain = nullptr;
	ID3D11DeviceContext* pContext = nullptr;
	void* presentAddress = nullptr;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,                       
		0, NULL, 0, D3D11_SDK_VERSION, &sd, 
		&pSwapChain, &pDevice, NULL, &pContext                   
	);

	if (FAILED(hr)) {
		Logger::LogAppend("ERROR: D3D11CreateDeviceAndSwapChain failed");
		goto cleanup;
	}

	pVMT = *reinterpret_cast<void***>(pSwapChain);
	presentAddress = pVMT[PRESENT_VMT_INDEX];

cleanup:
	if (pContext) pContext->Release();
	if (pDevice) pDevice->Release();
	if (pSwapChain) pSwapChain->Release();

	DestroyWindow(hWnd);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

	return presentAddress;
}

void Present_Hook::Install() {
    if (g_Present_Hook_Installed.load()) return;

    void* presentAddress = LocateDX11PresentAddress();
    if (!presentAddress) {
        Logger::LogAppend("Failed to obtain the address of Present()");
        return;
    }

    g_Present_Address = presentAddress;
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
		if (g_pState->gameHWND && original_WndProc)
		{
			SetWindowLongPtr(g_pState->gameHWND, GWLP_WNDPROC, (LONG_PTR)original_WndProc);
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