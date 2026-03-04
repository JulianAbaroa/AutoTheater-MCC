#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Hooks/Window/WndProcHook.h"
#include "External/imgui/imgui.h"
#include <chrono>

using namespace std::chrono_literals;

// Intercepts the window's message procedure to filter input events,
// granting ImGui priority and handling system-level lifecycle events.
LRESULT __stdcall WndProcHook::HookedWndProc(
	const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	// WM_CLOSE/WM_DESTROY/WM_QUIT: Intercept the window closing to clear the state.
	bool windowDestroyed = (uMsg == WM_CLOSE || uMsg == WM_DESTROY || uMsg == WM_QUIT);
	if (windowDestroyed)
	{
		if (g_pState->Lifecycle.IsRunning())
		{
			g_pUtil->Log.Append("[WndProc] WARNING: MCC shutdown detected.");

			g_pSystem->Preferences.SavePreferences();
			g_pState->Gallery.Cleanup();

			g_pSystem->Lifecycle.SignalShutdown();
		}
		
		return CallWindowProc(m_OriginalWndProc, hWnd, uMsg, wParam, lParam);
	}

	// WM_SYSKEYDOWN + VK_F4: Ignore Alt+F4 so that Windows handles it by default.
	bool altF4Pressed = (uMsg == WM_SYSKEYDOWN && wParam == VK_F4);
	if (altF4Pressed) return CallWindowProc(m_OriginalWndProc, hWnd, uMsg, wParam, lParam);

	// WM_KEYDOWN/WM_SYSKEYDOWN: It captures heartbeats before they reach the motor.
	bool isKeyDown = (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN);
	if (isKeyDown) if (HandleHotKeys(wParam)) return 0;

	// If the menu is visible, ImGui has input priority.
	if (g_pState->Settings.IsMenuVisible())
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) return 1;

		// WM_KEYFIRST/WM_KEYLAST: Range that covers from KeyDown to KeyUp.
		bool wantCaptureKeyboard = ImGui::GetIO().WantCaptureKeyboard;
		bool isKeyboardMessage = (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST);
		if (wantCaptureKeyboard && isKeyboardMessage) return 0;

		// Filter clicks and scrolling if the mouse is over an ImGui window.
		bool wantCaptureMouse = ImGui::GetIO().WantCaptureMouse;
		if (wantCaptureMouse)
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
	
	return CallWindowProc(m_OriginalWndProc, hWnd, uMsg, wParam, lParam);
}

// Private helper to handle mod-specific hotkeys.
bool WndProcHook::HandleHotKeys(WPARAM wParam)
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

WNDPROC WndProcHook::GetWndProc()
{
	return m_OriginalWndProc;
}

void WndProcHook::SetWndProc(WNDPROC lpPrevWndFunc)
{
	m_OriginalWndProc = lpPrevWndFunc;
}