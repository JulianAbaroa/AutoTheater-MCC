#pragma once

#include <winuser.h>

typedef UINT(WINAPI* GetRawInputData_t)(
	HRAWINPUT, UINT, LPVOID, PUINT, UINT
);

extern GetRawInputData_t original_GetRawInputData;
extern void* g_GetRawInputData_Address;

namespace GetRawInputData_Hook
{
	void Install();
	void Uninstall();

	UINT WINAPI hkGetRawInputData(
		HRAWINPUT hRawInput,
		UINT uiCommand,
		LPVOID pData,
		PUINT pcbSize,
		UINT cbSizeHeader
	);
}