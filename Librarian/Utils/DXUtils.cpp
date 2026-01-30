#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/DXUtils.h"
#include <sstream>
#include <d3d11.h>

DX11Addresses DXUtils::GetVtableAddresses() {
    DX11Addresses addr = { nullptr, nullptr };

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"DummyWindowClass";

    if (!RegisterClassEx(&wc)) {
        Logger::LogAppend("ERROR: RegisterClassEx failed");
        return addr;
    }

    HWND hWnd = CreateWindowEx(
        0, wc.lpszClassName, L"Dummy Window",
        WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
        NULL, NULL, wc.hInstance, NULL
    );

    if (!hWnd) {
        Logger::LogAppend("ERROR: CreateWindowEx failed");
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
        &pSwapChain, &pDevice, NULL, &pContext
    );

    if (SUCCEEDED(hr)) {
        void** pVMT = *reinterpret_cast<void***>(pSwapChain);
        addr.Present = pVMT[PRESENT_VMT_INDEX];
        addr.ResizeBuffers = pVMT[RESIZE_BUFFERS_VMT_INDEX];
    }
    else {
        Logger::LogAppend("ERROR: D3D11CreateDeviceAndSwapChain failed");
    }

    if (pContext) pContext->Release();
    if (pDevice) pDevice->Release();
    if (pSwapChain) pSwapChain->Release();

    DestroyWindow(hWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return addr;
}