#pragma once

#include <winuser.h>
#include <atomic>

class GetRawInputDataHook
{
public:
	void Install();
	void Uninstall();

private:
	static UINT WINAPI HookedGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData,
		PUINT pcbSize, UINT cbSizeHeader);

	typedef UINT(WINAPI* GetRawInputData_t)(HRAWINPUT, UINT, LPVOID, PUINT, UINT);

	static inline GetRawInputData_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};