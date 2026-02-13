#include "pch.h"
#include "Utils/Logger.h"
#include "RenderSystem.h"
#include "Core/Common/AppCore.h"
#include "Core/Hooks/UserInterface/WndProc_Hook.h"
#include "External/imgui/backends/imgui_impl_win32.h"
#include "External/imgui/backends/imgui_impl_dx11.h"
#include "External/imgui/imgui.h"

void RenderSystem::Initialize(IDXGISwapChain* pSwapChain)
{
    if (this->IsInitialized()) return;

    ID3D11Device* device = nullptr;
    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&device)))
    {
        ID3D11DeviceContext* context = nullptr;
        device->GetImmediateContext(&context);
    
        g_pState->Render.SetDevice(device);
        g_pState->Render.SetContext(context);
    
        DXGI_SWAP_CHAIN_DESC sd;
        pSwapChain->GetDesc(&sd);
        g_pState->Render.SetHWND(sd.OutputWindow);
    
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        float width = (float)sd.BufferDesc.Width;
        float baseFontSize = 22.0f;
        if (width >= 2560) baseFontSize = 30.0f;
        if (width >= 3840) baseFontSize = 38.0f;
    
        ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", baseFontSize);
        if (!font)
        {
            font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", baseFontSize);
        }
    
        ApplyCustomStyle();
    
        float scaleFactor = baseFontSize / 18.0f;
        ImGui::GetStyle().ScaleAllSizes(scaleFactor);
    
        ImGui_ImplWin32_Init(g_pState->Render.GetHWND());
        ImGui_ImplDX11_Init(device, context);
    
        original_WndProc = (WNDPROC)SetWindowLongPtr(g_pState->Render.GetHWND(), GWLP_WNDPROC, (LONG_PTR)WndProc_Hook::hkWndProc);
    
        context->Release();
        device->Release();
    }
}

bool RenderSystem::IsInitialized()
{
    return g_pState->Render.IsReady();
}

void RenderSystem::Shutdown()
{
    HWND hwnd = g_pState->Render.GetHWND();
    if (hwnd && original_WndProc)
    {
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)original_WndProc);
        original_WndProc = nullptr;
    }

    if (ImGui::GetCurrentContext())
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    g_pState->Render.FullCleanup();

    Logger::LogAppend("RenderSystem: Shutdown complete.");
}


void RenderSystem::BeginFrame(IDXGISwapChain* pSwapChain)
{
    if (g_pState->Render.GetRTV() == nullptr)
    {
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)))
        {
            ID3D11RenderTargetView* rtv = nullptr;
            g_pState->Render.GetDevice()->CreateRenderTargetView(pBackBuffer, NULL, &rtv);
            g_pState->Render.SetRTV(rtv);
            pBackBuffer->Release();
        }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool menuVisible = g_pState->Settings.IsMenuVisible();
    if (menuVisible)
    {
        if (g_pState->Settings.ShouldFreezeMouse() && GetForegroundWindow() == g_pState->Render.GetHWND()) {
            RECT rect;
            GetWindowRect(g_pState->Render.GetHWND(), &rect);
            ClipCursor(&rect);
        }

        ImGui::GetIO().MouseDrawCursor = true;
    }
    else {
        ClipCursor(NULL);
        ImGui::GetIO().MouseDrawCursor = false;
    }
}

void RenderSystem::EndFrame()
{
    ImGui::Render();

    if (auto rtv = g_pState->Render.GetRTV()) {
        auto ctx = g_pState->Render.GetContext();
        ID3D11RenderTargetView* oldRTV = nullptr;
        ID3D11DepthStencilView* oldDSV = nullptr;

        ctx->OMGetRenderTargets(1, &oldRTV, &oldDSV);
        ctx->OMSetRenderTargets(1, &rtv, NULL);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        ctx->OMSetRenderTargets(1, &oldRTV, oldDSV);

        if (oldRTV) oldRTV->Release();
        if (oldDSV) oldDSV->Release();
    }
}

void RenderSystem::ApplyCustomStyle()
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