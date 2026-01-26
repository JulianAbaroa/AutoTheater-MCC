#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/GlobalState.h"
#include "Hooks/UserInterface/WndProc_Hook.h"
#include "External/imgui/imgui_impl_win32.h"
#include "External/imgui/imgui.h"
#include <chrono>

using namespace std::chrono_literals;

WNDPROC original_WndProc = nullptr;

LRESULT __stdcall WndProc_Hook::hkWndProc(
	const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
) {
	if (uMsg == WM_CLOSE || uMsg == WM_DESTROY || uMsg == WM_QUIT)
	{
		if (g_pState->running.load())
		{
			Logger::LogAppend("EMERGENCY: App shutdown detected!");
			g_pState->running.store(false);
			g_pState->engineStatus.store({ EngineStatus::Destroyed });
			std::this_thread::sleep_for(200ms);
		}
		
		return CallWindowProc(original_WndProc, hWnd, uMsg, wParam, lParam);
	}

	if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)
	{
		bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

		if (ctrlPressed && wParam == 'A')
		{
			g_pState->showMenu.store(!g_pState->showMenu.load());
			return 0;
		}

		if (ctrlPressed && wParam == 'R')
		{
			g_pState->showMenu.store(true);
			g_pState->forceMenuReset.store(true);
			return 0;
		}
	}

	if (g_pState->showMenu.load())
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		{
			return 1;
		}

		if (ImGui::GetIO().WantCaptureKeyboard)
		{
			if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)
			{
				return 0;
			}
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