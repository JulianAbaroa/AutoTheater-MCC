#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "External/imgui/backends/imgui_impl_dx11.h"

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb/stb_image.h"

void RenderSystem::Initialize(IDXGISwapChain* pSwapChain)
{
    ID3D11Device* device = nullptr;
    if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&device))) return;

    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);

    g_pState->Render.SetDevice(device);
    g_pState->Render.SetContext(context);

    DXGI_SWAP_CHAIN_DESC sd;
    pSwapChain->GetDesc(&sd);
    g_pState->Render.SetHWND(sd.OutputWindow);

    if (!g_pState->Render.IsImGuiInitialized())
    {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        UINT width = sd.BufferDesc.Width;
        UINT height = sd.BufferDesc.Height;
        int evenWidth = static_cast<int>(width & ~1);
        int evenHeight = static_cast<int>(height & ~1);
        g_pState->Render.SetWidth(evenWidth);
        g_pState->Render.SetHeight(evenHeight);

        float baseFontSize = 22.0f;
        if (width >= 2560) baseFontSize = 30.0f;
        if (width >= 3840) baseFontSize = 38.0f;

        ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", baseFontSize);
        if (!font) font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", baseFontSize);

        ApplyCustomStyle();

        float scaleFactor = baseFontSize / 18.0f;
        ImGui::GetStyle().ScaleAllSizes(scaleFactor);

        ImGui_ImplWin32_Init(sd.OutputWindow);
        ImGui_ImplDX11_Init(device, context);

        g_pHook->WndProc.SetWndProc((WNDPROC)SetWindowLongPtr(sd.OutputWindow, GWLP_WNDPROC, (LONG_PTR)g_pHook->WndProc.HookedWndProc));

        g_pState->Render.SetImGuiInitialized(true);
        g_pUtil->Log.Append("[RenderSystem] INFO: ImGui Initialized for the first time.");
    }
    else
    {
        ImGui_ImplDX11_InvalidateDeviceObjects();
        ImGui_ImplDX11_CreateDeviceObjects();
        g_pUtil->Log.Append("[RenderSystem] INFO: Device Objects Refreshed (Resize).");
    }

    ID3D11Texture2D* pBackBuffer = nullptr;
    if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)))
    {
        ID3D11RenderTargetView* rtv = nullptr;
        device->CreateRenderTargetView(pBackBuffer, NULL, &rtv);
        g_pState->Render.SetRTV(rtv); 
        pBackBuffer->Release();
    }
}

bool RenderSystem::IsInitialized()
{
    return g_pState->Render.GetRTV() != nullptr;
}

void RenderSystem::Shutdown()
{
    HWND hwnd = g_pState->Render.GetHWND();
    WNDPROC original = g_pHook->WndProc.GetWndProc();

    if (hwnd && original)
    {
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)original);
        g_pHook->WndProc.SetWndProc(nullptr);
    }

    if (g_pState->Render.IsImGuiInitialized() && ImGui::GetCurrentContext())
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_pState->Render.SetImGuiInitialized(false);
    }

    g_pState->Render.FullCleanup();
    g_pUtil->Log.Append("[RenderSystem] INFO: Shutdown complete.");
}


void RenderSystem::UpdateFramerate()
{
    m_FrameCount++;
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - m_LastFramerateTime).count() >= 1)
    {
        g_pState->Render.SetFramerate(m_FrameCount.load());
        m_FrameCount = 0;
        m_LastFramerateTime = now;
    }
}

void RenderSystem::TickCapture(IDXGISwapChain* pSwapChain)
{
    if (!g_pState->FFmpeg.IsRecording())
    {
        m_LastCaptureTime = std::chrono::steady_clock::now();
        return;
    }

    float targetFPS = g_pState->FFmpeg.GetTargetFramerate();
    std::chrono::nanoseconds targetFrameDuration(static_cast<long long>(1000000000.0f / targetFPS));

    auto now = std::chrono::steady_clock::now();

    if (now - m_LastCaptureTime >= targetFrameDuration)
    {
        this->CaptureFrame(pSwapChain);

        m_LastCaptureTime += targetFrameDuration;

        if (now - m_LastCaptureTime > targetFrameDuration)
        {
            m_LastCaptureTime = now;
        }
    }
}

void RenderSystem::CaptureFrame(IDXGISwapChain* pSwapChain)
{
    if (!g_pState->Video.IsRecording()) return;

    ID3D11Device* device = g_pState->Render.GetDevice();
    ID3D11DeviceContext* context = g_pState->Render.GetContext();

    ID3D11Texture2D* pBackBuffer = nullptr;
    if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)))
    {
        return;
    }

    D3D11_TEXTURE2D_DESC desc;
    pBackBuffer->GetDesc(&desc);

    for (int i = 0; i < 2; i++)
    {
        if (!m_pStagingTextures[i])
        {
            D3D11_TEXTURE2D_DESC stagingDesc = desc;
            stagingDesc.Usage = D3D11_USAGE_STAGING;
            stagingDesc.BindFlags = 0;
            stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            stagingDesc.MiscFlags = 0;
            device->CreateTexture2D(&stagingDesc, nullptr, &m_pStagingTextures[i]);
        }
    }

    context->CopyResource(m_pStagingTextures[m_currentBufferIndex], pBackBuffer);
    pBackBuffer->Release();

    int prevIndex = (m_currentBufferIndex + 1) % 2;

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(context->Map(m_pStagingTextures[prevIndex], 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped)))
    {
        float* pTime = g_pState->Theater.GetTimePtr();
        float currentTime = (pTime) ? *pTime : 0.0f;

        g_pSystem->Video.PushFrame((uint8_t*)mapped.pData, desc.Width, desc.Height, mapped.RowPitch, currentTime);

        context->Unmap(m_pStagingTextures[prevIndex], 0);
    }

    m_currentBufferIndex = prevIndex;
}


void RenderSystem::BeginFrame(IDXGISwapChain* pSwapChain)
{
    if (g_pState->Render.GetRTV() == nullptr)
    {
        this->Initialize(pSwapChain);
        if (g_pState->Render.GetRTV() == nullptr) return;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool menuVisible = g_pState->Settings.IsMenuVisible();
    if (menuVisible)
    {
        if (g_pState->Settings.ShouldFreezeMouse() && GetForegroundWindow() == g_pState->Render.GetHWND()) 
        {
            RECT rect;
            GetWindowRect(g_pState->Render.GetHWND(), &rect);
            ClipCursor(&rect);
        }
        ImGui::GetIO().MouseDrawCursor = true;
    }
    else 
    {
        if (ImGui::GetIO().MouseDrawCursor) 
        {
            ClipCursor(NULL);
            ImGui::GetIO().MouseDrawCursor = false;
        }
    }
}

void RenderSystem::EndFrame()
{
    ImGui::Render();

    auto rtv = g_pState->Render.GetRTV();
    if (rtv)
    {
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


void* RenderSystem::CreateTextureFromMemory(const unsigned char* data, size_t size)
{
    ID3D11Device* device = g_pState->Render.GetDevice();
    if (!device) return nullptr;

    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 4);
    if (!pixels) return nullptr;

    ID3D11Texture2D* pTexture = nullptr;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = pixels;
    subResource.SysMemPitch = width * 4;

    device->CreateTexture2D(&desc, &subResource, &pTexture);

    ID3D11ShaderResourceView* srv = nullptr;
    if (pTexture) 
    {
        device->CreateShaderResourceView(pTexture, nullptr, &srv);
        pTexture->Release();
    }

    stbi_image_free(pixels);
    return (void*)srv;
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