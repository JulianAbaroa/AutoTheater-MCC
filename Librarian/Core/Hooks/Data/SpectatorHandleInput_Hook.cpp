#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Common/AppCore.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include "Core/Hooks/Data/SpectatorHandleInput_Hook.h"
#include "External/minhook/include/MinHook.h"

SpectatorHandleInput_t original_SpectatorHandleInput = nullptr;
std::atomic<bool> g_SpectatorHandleInput_Hook_Installed = false;
void* g_SpectatorHandleInput_Address = nullptr;

void __fastcall hkSpectatorHandleInput(
	uint64_t* pReplayModule,
	uint32_t rdx_param
) {
	original_SpectatorHandleInput(pReplayModule, rdx_param);

	if (pReplayModule != nullptr)
	{		
		g_pState->Theater.SetReplayModule(reinterpret_cast<uintptr_t>(pReplayModule));
		g_pState->Theater.SetCameraMode(*reinterpret_cast<uint8_t*>(g_pState->Theater.GetReplayModule() + 0x190));

		g_pSystem->Theater.TryGetSpectatedPlayerIndex(g_pState->Theater.GetReplayModule());

		uint8_t followedPlayerIdx = g_pState->Theater.GetSpectatedPlayerIndex();

		if (followedPlayerIdx < 16)
		{
			static uint8_t lastID = 255;

			if (followedPlayerIdx != lastID)
			{
				lastID = followedPlayerIdx;

				wchar_t bufferNombre[32] = { 0 };
				if (g_pSystem->Theater.TryGetPlayerName(followedPlayerIdx, bufferNombre, 32)) {
					std::string sName = Formatting::WStringToString(bufferNombre);

					std::stringstream ss;
					ss << ">>> Changed to: [" << (int)followedPlayerIdx << "] " << sName;
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