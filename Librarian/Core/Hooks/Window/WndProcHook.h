#pragma once

#include "External/imgui/backends/imgui_impl_win32.h"
#include <Windows.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class WndProcHook
{
public:
	static LRESULT __stdcall HookedWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	static WNDPROC GetWndProc();
	static void SetWndProc(WNDPROC lpPrevWndFunc);

private:
	static bool HandleHotKeys(WPARAM wParam);

	static inline WNDPROC m_OriginalWndProc = nullptr;
};