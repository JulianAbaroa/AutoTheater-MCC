#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/DXUtils.h"
#include "Core/Common/AppCore.h"
#include "Core/UserInterface/UserInterface.h"
#include "Core/Hooks/UserInterface/Present_Hook.h"
#include "Core/Hooks/UserInterface/GetRawInputData_Hook.h"
#include "External/minhook/include/MinHook.h"
#include <sstream>
#pragma comment(lib, "d3d11.lib")

Present_t original_Present = nullptr;
std::atomic<bool> g_Present_Hook_Installed = false;
void* g_Present_Address = nullptr;

static HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) 
{
    if (!g_pSystem->Render.IsInitialized())
    {
        g_pSystem->Render.Initialize(pSwapChain);
    }

    g_pSystem->Render.BeginFrame(pSwapChain);

    UserInterface::DrawMainInterface();

    g_pSystem->Render.EndFrame();

    if (g_pState->Theater.IsTheaterMode()) {
        g_pSystem->Theater.UpdateRealTimeScale();
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

    g_pSystem->Render.Shutdown();

    if (g_Present_Address)
    {
        MH_DisableHook(g_Present_Address);
        MH_RemoveHook(g_Present_Address);
    }

    GetRawInputData_Hook::Uninstall();

    g_Present_Hook_Installed.store(false);
    Logger::LogAppend("Present hook uninstalled successfully");
}