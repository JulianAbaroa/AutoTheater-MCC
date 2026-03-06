#pragma once

#include <shlwapi.h>
#include <thread>

class AppLoader
{
public:
	BOOL OnAttach(HMODULE hModule);
	void OnDetach(LPVOID lpReserved);

private:
	static DWORD WINAPI InitializeLibrarian(LPVOID lpParam);
	void DeinitializeLibrarian(LPVOID lpReserved);

	std::thread m_MainThread{};
	std::thread m_DirectorThread{};
	std::thread m_InputThread{};
	std::thread m_CaptureThread{};
};