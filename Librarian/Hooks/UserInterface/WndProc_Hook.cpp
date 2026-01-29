/**
 * @file WindProc_Hook.cpp
 * @brief Window Procedure Hooking logic.
 * * Intercepts Window messages to handle hotkeys, ImGui input capture,
 * and graceful shutdowns when the game window closes.
 */

#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/GlobalState.h"
#include "Hooks/UserInterface/WndProc_Hook.h"
#include "External/imgui/backends/imgui_impl_win32.h"
#include "External/imgui/imgui.h"
#include <chrono>

using namespace std::chrono_literals;

WNDPROC original_WndProc = nullptr;

/** 
 * @brief Private helper to handle mod-specific hotkeys.
 * @return true if the message was handled and should not be passed to the game.
 */
bool HandleHotKeys(WPARAM wParam)
{
	bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

	if (ctrlPressed && wParam == '1')
	{
		g_pState->showMenu.store(!g_pState->showMenu.load());
		return true;
	}

	if (ctrlPressed && wParam == '2')
	{
		g_pState->showMenu.store(true);
		g_pState->forceMenuReset.store(true);
		return true;
	}

	return false;
}

/**
 * @brief Main Window Procedure Hook.
 * @details Orchestrate the priority of messages:
 * 1. System shutdown (High Priority).
 * 2. Mod Hotkeys.
 * 3. ImGui Input (If the menu is visible).
 * 4. Game (Original WndProc).
 */
LRESULT __stdcall WndProc_Hook::hkWndProc(
	const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
) {
	// 1. Closure Management.
	if (uMsg == WM_CLOSE || uMsg == WM_DESTROY || uMsg == WM_QUIT)
	{
		if (g_pState->running.load())
		{
			Logger::LogAppend("EMERGENCY: App shutdown detected!");
			g_pState->running.store(false);
			g_pState->shutdownCV.notify_all();
		}
		
		return CallWindowProc(original_WndProc, hWnd, uMsg, wParam, lParam);
	}

	// 2. Hotkey Management.
	if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)
	{
		if (HandleHotKeys(wParam)) return 0;
	}

	// 3. Interface Management (ImGui).
	if (g_pState->showMenu.load())
	{
		// Send to ImGui.
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		{
			return 1;
		}

		// Block game input if ImGui requires it
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
	
	// 4. Send to the game.
	return CallWindowProc(original_WndProc, hWnd, uMsg, wParam, lParam);
}