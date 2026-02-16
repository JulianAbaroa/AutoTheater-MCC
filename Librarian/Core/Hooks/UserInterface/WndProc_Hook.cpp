#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/AppCore.h"
#include "Core/Hooks/UserInterface/WndProc_Hook.h"
#include "External/imgui/backends/imgui_impl_win32.h"
#include "External/imgui/imgui.h"
#include <chrono>

using namespace std::chrono_literals;

WNDPROC original_WndProc = nullptr;

// Private helper to handle mod-specific hotkeys.
bool HandleHotKeys(WPARAM wParam)
{
	bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

	if (ctrlPressed && wParam == '1')
	{
		g_pState->Settings.SetMenuVisible(!g_pState->Settings.IsMenuVisible());
		return true;
	}

	if (ctrlPressed && wParam == '2')
	{
		g_pState->Settings.SetMenuVisible(true);
		g_pState->Settings.SetForceMenuReset(true);
		return true;
	}

	return false;
}

LRESULT __stdcall WndProc_Hook::hkWndProc(
	const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
) {
	if (uMsg == WM_CLOSE || uMsg == WM_DESTROY || uMsg == WM_QUIT)
	{
		if (g_pState->Lifecycle.IsRunning())
		{
			Logger::LogAppend("EMERGENCY: App shutdown detected!");
			g_pSystem->Lifecycle.SignalShutdown();
		}
		
		return CallWindowProc(original_WndProc, hWnd, uMsg, wParam, lParam);
	}

	if (uMsg == WM_SYSKEYDOWN && wParam == VK_F4)
	{
		return CallWindowProc(original_WndProc, hWnd, uMsg, wParam, lParam);
	}

	if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)
	{
		if (HandleHotKeys(wParam)) return 0;
	}

	if (g_pState->Settings.IsMenuVisible())
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		{
			return 1;
		}

		if (ImGui::GetIO().WantCaptureKeyboard && (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST))
		{
			return 0;
		}

		if (ImGui::GetIO().WantCaptureMouse)
		{
			switch (uMsg)
			{
			case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
			case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
				return 0;
			}
		}
	}
	
	return CallWindowProc(original_WndProc, hWnd, uMsg, wParam, lParam);
}