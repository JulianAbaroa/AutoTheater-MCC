#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/GlobalState.h"
#include "Hooks/UserInterface/GetRawInputData_Hook.h"
#include "External/minhook/include/MinHook.h"

GetRawInputData_t original_GetRawInputData = nullptr;
std::atomic<bool> g_GetRawInputData_Hook_Installed{ false };
void* g_GetRawInputData_Address = nullptr;

UINT WINAPI GetRawInputData_Hook::hkGetRawInputData(
	HRAWINPUT hRawInput, 
	UINT uiCommand, 
	LPVOID pData, 
	PUINT pcbSize, 
	UINT cbSizeHeader
) {
	UINT dwSize = original_GetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

	if (dwSize != (UINT)-1 && pData != NULL && 
		g_pState->ShowMenu.load() && g_pState->FreezeMouse.load()
	) {
		RAWINPUT* raw = (RAWINPUT*)pData;
		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			raw->data.mouse.lLastX = 0;
			raw->data.mouse.lLastY = 0;

			raw->data.mouse.ulButtons = 0;
			raw->data.mouse.usButtonFlags = 0;
		}
	}

	return dwSize;
}

void GetRawInputData_Hook::Install()
{
	if (g_GetRawInputData_Hook_Installed.load()) return;

	HMODULE hUser32 = GetModuleHandle(L"user32.dll");

	if (!hUser32) {
		Logger::LogAppend("ERROR: Could not get handle for user32.dll");
		return;
	}

	g_GetRawInputData_Address = (void*)GetProcAddress(hUser32, "GetRawInputData");
	if (!g_GetRawInputData_Address) {
		Logger::LogAppend("ERROR: GetProcAddress for GetRawInputData failed");
		return;
	}

	if (MH_CreateHook(
		g_GetRawInputData_Address, &GetRawInputData_Hook::hkGetRawInputData,
		reinterpret_cast<LPVOID*>(&original_GetRawInputData)) != MH_OK
		) {
		Logger::LogAppend("Failed to create GetRawInputData hook");
	}
	
	if (MH_EnableHook(g_GetRawInputData_Address) != MH_OK) {
		Logger::LogAppend("Failed to enable GetRawInputData hook");
		return;
	}
	
	g_GetRawInputData_Hook_Installed.store(true);
	Logger::LogAppend("GetRawInputData hook installed successfully");
}

void GetRawInputData_Hook::Uninstall()
{
	if (!g_GetRawInputData_Hook_Installed.load()) return;

	MH_DisableHook(g_GetRawInputData_Address);
	MH_RemoveHook(g_GetRawInputData_Address);

	g_GetRawInputData_Hook_Installed.store(false);
	Logger::LogAppend("GetRawInputData hook uninstalled");
}