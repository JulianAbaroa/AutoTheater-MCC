#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Theater/TheaterSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Infrastructure/Engine/FormatSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Hooks/Data/SpectatorHandleInputHook.h"
#include "External/minhook/include/MinHook.h"

// This function handles the logic for spectator input processing and state transitions.
// It is called frequently (matching the engine's input polling rate) to update 
// the Spectator/Replay Module state.
// AutoTheater uses this hook to:
// 1. Module Tracking: Captures the 'pReplayModule' pointer, which serves as the 
//    central controller for camera modes and spectated targets.
// 2. Camera Monitoring: Reads the current Camera Mode (at offset +0x190) to 
//    detect if the user is in Free Camera, First Person, or Third Person.
// 3. Target Tracking: Monitors changes in the spectated player index. When a 
//    change is detected, it resolves the player's name for logging and 
//    synchronizes the Director's focus target.
void __fastcall SpectatorHandleInputHook::HookedSpectatorHandleInput(
	ReplayModule* pReplayModule, 
	uint32_t rdx_param) 
{
	m_OriginalFunction(pReplayModule, rdx_param);

	if (pReplayModule != nullptr) 
	{	
		g_pState->Domain->Theater->UpdateReplayModule(pReplayModule);

		uint8_t followedPlayerIdx = (uint8_t)pReplayModule->FollowedPlayerIndex;
		if (followedPlayerIdx < 16)
		{
			static uint8_t lastID = 255;

			if (followedPlayerIdx != lastID)
			{
				lastID = followedPlayerIdx;
				wchar_t nameBuffer[32] = { 0 };

				if (g_pSystem->Domain->Theater->TryGetPlayerName(followedPlayerIdx, nameBuffer, 32))
				{
					std::string sName = g_pSystem->Infrastructure->Format->WStringToString(nameBuffer);
					g_pSystem->Debug->Log("[SpectatorHandleInput] INFO: Changed to: [%d] %s", followedPlayerIdx, sName.c_str());
				}
			}
		}
	}
}


void SpectatorHandleInputHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = (void*)g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::SpectatorHandleInput);
	if (!functionAddress)
	{
		g_pSystem->Debug->Log("[SpectatorHandleInput] ERROR: Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedSpectatorHandleInput, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pSystem->Debug->Log("[SpectatorHandleInput] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pSystem->Debug->Log("[SpectatorHandleInput] ERROR: Failed to enable the hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[SpectatorHandleInput] INFO: Hook installed.");
}

void SpectatorHandleInputHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[SpectatorHandleInput] INFO: Hook uninstalled.");
}