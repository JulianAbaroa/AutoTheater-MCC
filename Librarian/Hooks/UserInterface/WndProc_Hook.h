#pragma once

#include <Windows.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam
);

extern WNDPROC original_WndProc;

namespace WndProc_Hook
{
	LRESULT __stdcall hkWndProc(
		const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
	);
}