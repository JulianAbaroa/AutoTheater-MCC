#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Core/Scanner/Scanner.h"
#include "Core/Systems/Timeline.h"
#include "Hooks/Data/SpectatorHandleInput_Hook.h"

SpectatorHandleInput_t original_SpectatorHandleInput = nullptr;
std::atomic<bool> g_SpectatorHandleInput_Hook_Installed = false;
void* g_SpectatorHandleInput_Address = nullptr;

void __fastcall hkSpectatorHandleInput(
	uintptr_t pReplayModule,
	uintptr_t rdx_param
) {
	original_SpectatorHandleInput((uint64_t*)pReplayModule, rdx_param);

	if (pReplayModule > 0x10000)
	{
		g_pReplayModule = pReplayModule;

		Theater::TryGetFollowedPlayerIdx(pReplayModule);

		if (g_FollowedPlayerIdx < 16)
		{
			static uint8_t lastID = 255;

			if (g_FollowedPlayerIdx != lastID)
			{
				lastID = g_FollowedPlayerIdx;

				wchar_t bufferNombre[32] = { 0 };
				if (Theater::TryGetPlayerName(g_FollowedPlayerIdx, bufferNombre, 32)) {
					std::wstring ws(bufferNombre);
					std::string sName(ws.begin(), ws.end());

					std::stringstream ss;
					ss << ">>> Changed to: [" << (int)g_FollowedPlayerIdx << "] " << sName;
					Logger::LogAppend(ss.str().c_str());
				}
			}
		}
	}
}

void SpectatorHandleInput_Hook::Install()
{
	if (g_SpectatorHandleInput_Hook_Installed.load()) return;

	void* methodAddress = (void*)Scanner::FindPattern(Sig_SpectatorHandleInput);
	if (!methodAddress)
	{
		Logger::LogAppend("Failed to obtain the address of SpectatorHandleInput()");
		return;
	}

	g_SpectatorHandleInput_Address = methodAddress;

	if (MH_CreateHook(g_SpectatorHandleInput_Address, &hkSpectatorHandleInput, reinterpret_cast<LPVOID*>(&original_SpectatorHandleInput)) != MH_OK)
	{
		Logger::LogAppend("Failed to create SpectatorHandleInput hook");
		return;
	}

	if (MH_EnableHook(g_SpectatorHandleInput_Address) != MH_OK)
	{
		Logger::LogAppend("Failed to enable SpectatorHandleInput hook");
		return;
	}

	g_SpectatorHandleInput_Hook_Installed.store(true);
	Logger::LogAppend("SpectatorHandleInput hook installed");
}

void SpectatorHandleInput_Hook::Uninstall()
{
	if (!g_SpectatorHandleInput_Hook_Installed.load()) return;

	HMODULE hMod = GetModuleHandle(L"haloreach.dll");
	if (hMod != nullptr && g_SpectatorHandleInput_Address != 0)
	{
		MH_DisableHook(g_SpectatorHandleInput_Address);
		MH_RemoveHook(g_SpectatorHandleInput_Address);
		Logger::LogAppend("SpectatorHandleInput hook uninstalled safely");
	}
	else
	{
		Logger::LogAppend("SpectatorHandleInput hook skipped uninstall (module already gone)");
	}

	g_SpectatorHandleInput_Hook_Installed.store(false);
}