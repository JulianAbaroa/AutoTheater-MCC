#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Hooks/Input/GetRawInputDataHook.h"
#include "External/minhook/include/MinHook.h"

// Intercepts Windows Raw Input messages to prevent the game from processing 
// mouse movement and clicks when the ImGui menu is active.
UINT WINAPI GetRawInputDataHook::HookedGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, 
	LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) 
{
	UINT dwSize = m_OriginalFunction(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

	if (dwSize != (UINT)-1 && pData != NULL && 
		g_pState->Settings.IsMenuVisible() && 
		g_pState->Settings.ShouldFreezeMouse()) 
	{
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

void GetRawInputDataHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	if (!hUser32) 
	{
		g_pUtil->Log.Append("[GetRawInputData] ERROR: Could not get handle for user32.dll");
		return;
	}

	m_FunctionAddress.store((void*)GetProcAddress(hUser32, "GetRawInputData"));
	if (!m_FunctionAddress.load()) 
	{
		g_pUtil->Log.Append("[GetRawInputData] ERROR: GetProcAddress for GetRawInputData failed");
		return;
	}
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedGetRawInputData, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pUtil->Log.Append("[GetRawInputData] ERROR: Failed to create the hook.");
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		g_pUtil->Log.Append("[GetRawInputData] ERROR: Failed to enable the hook.");
		return;
	}
	
	m_IsHookInstalled.store(true);
	g_pUtil->Log.Append("[GetRawInputData] INFO: Hook installed.");
}

void GetRawInputDataHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pUtil->Log.Append("[GetRawInputData] INFO: Hook uninstalled.");
}