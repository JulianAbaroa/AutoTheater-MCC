#include "pch.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Window/CoreWindowHook.h"
#include "Core/Hooks/Window/WndProcHook.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Capture/VideoState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"
#include "Core/Systems/Infrastructure/Engine/RenderSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "External/imgui/backends/imgui_impl_dx11.h"

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb/stb_image.h"

DX11Addresses RenderSystem::GetVtableAddresses()
{
    DX11Addresses addr = { nullptr, nullptr };

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"DummyWindowClass";

    if (!RegisterClassEx(&wc))
    {
        g_pSystem->Debug->Log("[DXUtil] ERROR: RegisterClassEx failed");
        return addr;
    }

    HWND hWnd = CreateWindowEx(
        0, wc.lpszClassName, L"Dummy Window",
        WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
        NULL, NULL, wc.hInstance, NULL);

    if (!hWnd)
    {
        g_pSystem->Debug->Log("[DXUtil] ERROR: CreateWindowEx failed");
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return addr;
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

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        0, NULL, 0, D3D11_SDK_VERSION, &sd,
        &pSwapChain, &pDevice, NULL, &pContext);

    if (SUCCEEDED(hr))
    {
        void** pVMT = *reinterpret_cast<void***>(pSwapChain);
        addr.Present = pVMT[m_PresentVMTIndex];
        addr.ResizeBuffers = pVMT[m_ResizeBuffersVMTIndex];
    }
    else
    {
        g_pSystem->Debug->Log("[DXUtil] ERROR: D3D11CreateDeviceAndSwapChain failed");
    }

    if (pContext) pContext->Release();
    if (pDevice) pDevice->Release();
    if (pSwapChain) pSwapChain->Release();

    DestroyWindow(hWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return addr;
}

void RenderSystem::Initialize(IDXGISwapChain* pSwapChain)
{
    ID3D11Device* device = nullptr;
    if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&device))) return;

    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);

    g_pState->Infrastructure->Render->SetDevice(device);
    g_pState->Infrastructure->Render->SetContext(context);

    DXGI_SWAP_CHAIN_DESC sd;
    pSwapChain->GetDesc(&sd);
    g_pState->Infrastructure->Render->SetHWND(sd.OutputWindow);

    if (!g_pState->Infrastructure->Render->IsImGuiInitialized())
    {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        UINT width = sd.BufferDesc.Width;
        UINT height = sd.BufferDesc.Height;
        int evenWidth = static_cast<int>(width & ~1);
        int evenHeight = static_cast<int>(height & ~1);
        g_pState->Infrastructure->Render->SetWidth(evenWidth);
        g_pState->Infrastructure->Render->SetHeight(evenHeight);

        float baseFontSize = 22.0f;
        if (width >= 2560) baseFontSize = 30.0f;
        if (width >= 3840) baseFontSize = 38.0f;

        float savedScale = g_pState->Infrastructure->Render->GetUIScale();
        if (savedScale < 0.8f || savedScale > 4.0f) savedScale = 1.0f;

        float finalFontSize = baseFontSize * savedScale;

        ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", finalFontSize);
        if (!font) font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", finalFontSize);

        this->ApplyCustomStyle();

        if (g_pState->Infrastructure->Settings->ShouldUseAppData())
        {
            ImGui::GetStyle().Alpha = g_pState->Infrastructure->Settings->GetMenuAlpha();
        }

        ImGui::GetStyle().ScaleAllSizes(savedScale);

        ImGui_ImplWin32_Init(sd.OutputWindow);
        ImGui_ImplDX11_Init(device, context);

        g_pHook->Window->WndProc->SetWndProc((WNDPROC)SetWindowLongPtr(sd.OutputWindow, GWLP_WNDPROC, (LONG_PTR)g_pHook->Window->WndProc->HookedWndProc));
        
        bool showOnStart = g_pState->Infrastructure->Settings->ShouldOpenUIOnStart();
        g_pState->Infrastructure->Settings->SetMenuVisible(showOnStart);

        g_pState->Infrastructure->Render->SetImGuiInitialized(true);
        g_pSystem->Debug->Log("[RenderSystem] INFO: ImGui Initialized for the first time.");
    }
    else
    {
        ImGui_ImplDX11_InvalidateDeviceObjects();
        ImGui_ImplDX11_CreateDeviceObjects();
        g_pSystem->Debug->Log("[RenderSystem] INFO: Device Objects Refreshed (Resize).");
    }

    ID3D11Texture2D* pBackBuffer = nullptr;
    if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)))
    {
        ID3D11RenderTargetView* rtv = nullptr;
        device->CreateRenderTargetView(pBackBuffer, NULL, &rtv);
        g_pState->Infrastructure->Render->SetRTV(rtv); 
        pBackBuffer->Release();
    }
}

bool RenderSystem::IsInitialized()
{
    return g_pState->Infrastructure->Render->GetRTV() != nullptr;
}

void RenderSystem::Shutdown()
{
    HWND hwnd = g_pState->Infrastructure->Render->GetHWND();
    WNDPROC original = g_pHook->Window->WndProc->GetWndProc();

    if (hwnd && original)
    {
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)original);
        g_pHook->Window->WndProc->SetWndProc(nullptr);
    }

    if (g_pState->Infrastructure->Render->IsImGuiInitialized() && ImGui::GetCurrentContext())
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_pState->Infrastructure->Render->SetImGuiInitialized(false);
    }

    g_pState->Infrastructure->Render->FullCleanup();
    g_pSystem->Debug->Log("[RenderSystem] INFO: Shutdown complete.");
}


void RenderSystem::UpdateFramerate()
{
    m_FrameCount++;
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - m_LastFramerateTime).count() >= 1)
    {
        g_pState->Infrastructure->Render->SetFramerate(m_FrameCount.load());
        m_FrameCount = 0;
        m_LastFramerateTime = now;
    }
}

void RenderSystem::TickCapture(IDXGISwapChain* pSwapChain)
{
    if (!g_pState->Infrastructure->FFmpeg->IsRecording())
    {
        m_LastCaptureTime = std::chrono::steady_clock::now();
        return;
    }

    this->CaptureFrame(pSwapChain);
}

void RenderSystem::CaptureFrame(IDXGISwapChain* pSwapChain)
{
    if (!g_pState->Infrastructure->Video->IsRecording()) return;

    ID3D11Device* device = g_pState->Infrastructure->Render->GetDevice();
    ID3D11DeviceContext* context = g_pState->Infrastructure->Render->GetContext();

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
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - g_pState->Infrastructure->FFmpeg->GetStartRecordingTime();
        double currentRealTime = elapsed.count();

        g_pSystem->Infrastructure->Video->PushFrame((uint8_t*)mapped.pData, desc.Width, desc.Height, mapped.RowPitch, currentRealTime);
        
        context->Unmap(m_pStagingTextures[prevIndex], 0);
    }

    m_currentBufferIndex = prevIndex;
}


void RenderSystem::BeginFrame(IDXGISwapChain* pSwapChain)
{
    if (g_pState->Infrastructure->Render->GetRTV() == nullptr)
    {
        this->Initialize(pSwapChain);
        if (g_pState->Infrastructure->Render->GetRTV() == nullptr) return;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool menuVisible = g_pState->Infrastructure->Settings->IsMenuVisible();
    ImGuiIO& io = ImGui::GetIO();

    if (menuVisible)
    {
        if (g_pState->Infrastructure->Settings->ShouldFreezeMouse() && GetForegroundWindow() == g_pState->Infrastructure->Render->GetHWND()) 
        {
            RECT rect;
            GetWindowRect(g_pState->Infrastructure->Render->GetHWND(), &rect);
            ClipCursor(&rect);
        }
        
        io.MouseDrawCursor = true;
    }
    else 
    {
        if (io.MouseDrawCursor)
        {
            ClipCursor(NULL);
            io.MouseDrawCursor = false;
        }
    }
}

void RenderSystem::EndFrame()
{
    ImGui::Render();

    auto rtv = g_pState->Infrastructure->Render->GetRTV();
    if (rtv)
    {
        auto ctx = g_pState->Infrastructure->Render->GetContext();

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
    ID3D11Device* device = g_pState->Infrastructure->Render->GetDevice();
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


void RenderSystem::UpdateUIScale()
{
    if (!g_pState->Infrastructure->Render->ShouldRebuildFonts()) return;

    float newScale = g_pState->Infrastructure->Render->GetUIScale();
    if (newScale < 1.0f) newScale = 1.0f;

    UINT width = g_pState->Infrastructure->Render->GetWidth();
    float baseFontSize = 22.0f;
    if (width >= 2560) baseFontSize = 30.0f;
    if (width >= 3840) baseFontSize = 38.0f;

    float finalFontSize = baseFontSize * newScale;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 oldDisplaySize = io.DisplaySize;

    io.Fonts->Clear();

    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", finalFontSize);
    if (!font) io.Fonts->AddFontDefault();

    ImGui_ImplDX11_InvalidateDeviceObjects();
    ImGui_ImplDX11_CreateDeviceObjects();

    io.DisplaySize = oldDisplaySize;

    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();
    this->ApplyCustomStyle();

    style.ScaleAllSizes(newScale);
    style.Alpha = g_pState->Infrastructure->Settings->GetMenuAlpha();

    g_pState->Infrastructure->Render->ResetFontRebuild();
}


void RenderSystem::ApplyCustomStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style = ImGuiStyle();

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