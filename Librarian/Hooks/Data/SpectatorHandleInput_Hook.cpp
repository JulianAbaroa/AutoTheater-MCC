#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Scanner/Scanner.h"
#include "Core/Systems/Timeline.h"
#include "Core/Systems/Director.h"
#include "Hooks/Data/SpectatorHandleInput_Hook.h"
#include "External/minhook/include/MinHook.h"

SpectatorHandleInput_t original_SpectatorHandleInput = nullptr;
std::atomic<bool> g_SpectatorHandleInput_Hook_Installed = false;
void* g_SpectatorHandleInput_Address = nullptr;

volatile uint8_t g_FollowedPlayerIdx = 255;
uintptr_t g_pReplayModule = 0;
uint8_t g_CameraAttached = 0xFF;

void __fastcall hkSpectatorHandleInput(
	uint64_t* pReplayModule,
	uint32_t rdx_param
) {
	original_SpectatorHandleInput(pReplayModule, rdx_param);

	if (pReplayModule != nullptr)
	{
		g_pReplayModule = reinterpret_cast<uintptr_t>(pReplayModule);
		g_CameraAttached = *reinterpret_cast<uint8_t*>(g_pReplayModule + 0x190);

		Theater::TryGetFollowedPlayerIdx(g_pReplayModule);

		if (g_FollowedPlayerIdx < 16)
		{
			static uint8_t lastID = 255;

			if (g_FollowedPlayerIdx != lastID)
			{
				lastID = g_FollowedPlayerIdx;

				wchar_t bufferNombre[32] = { 0 };
				if (Theater::TryGetPlayerName(g_FollowedPlayerIdx, bufferNombre, 32)) {
					std::string sName = Formatting::WStringToString(bufferNombre);

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

	void* methodAddress = (void*)Scanner::FindPattern(Signatures::SpectatorHandleInput);
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

	MH_DisableHook(g_SpectatorHandleInput_Address);
	MH_RemoveHook(g_SpectatorHandleInput_Address);

	g_SpectatorHandleInput_Hook_Installed.store(false);
	Logger::LogAppend("SpectatorHandleInput hook uninstalled");
}