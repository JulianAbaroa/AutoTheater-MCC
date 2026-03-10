#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Timeline/TimelineState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Persistence/ReplaySystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Domain/MainThread.h"
#include "Core/Hooks/Data/BlamOpenFileHook.h"
#include "External/minhook/include/MinHook.h"

// This function is a hook for the Blam! File System API, used for interacting with Windows I/O.
// It is triggered during engine initialization and state transitions to load maps,
// game variants, and resource packages.
// AutoTheater Interception: By hooking this call, the mod identifies the specific replay
// file (.mov) being loaded by the player. This allows AutoTheater to:
// 1. Scan and extract 'CHDR' (Replay Metadata) from the disk.
// 2. Hash the file to ensure the currently loaded Timeline matches the replay data.
// 3. Synchronize the Director's lifecycle with the game's file-loading state.
void BlamOpenFileHook::HookedBlamOpenFile(
	long long fileContext, uint32_t accessFlags, uint32_t* translatedStatus)
{
	m_OriginalFunction(fileContext, accessFlags, translatedStatus);

	const char* filePath = (const char*)((uintptr_t)fileContext + 0x8);

	if (filePath != nullptr && !IsBadReadPtr(filePath, 4)) 
	{
		std::string pathStr(filePath);
		if (pathStr.find(".mov") != std::string::npos) 
		{
			TheaterReplay replay = g_pSystem->Infrastructure->Replay->ScanReplay(filePath);

			std::string currentFileHash = g_pSystem->Infrastructure->Replay->CalculateFileHash(filePath);

			if (g_pState->Infrastructure->Lifecycle->GetCurrentPhase() == Phase::Timeline)
			{
				g_pUtil->Log.Append("[BlamOpenFile] INFO: Replay path: %s", filePath);

				g_pState->Domain->Timeline->SetAssociatedReplayHash(currentFileHash);
				g_pUtil->Log.Append("[BlamOpenFile] INFO: Timeline bound to Replay Hash: %s", currentFileHash.substr(0, 8).c_str());

				g_pUtil->Log.Append("[BlamOpenFile] INFO: Recorded by: %s", replay.ReplayMetadata.Author);

				if (!replay.ReplayMetadata.Info.empty())
				{
					g_pUtil->Log.Append("[BlamOpenFile] INFO: %s", replay.ReplayMetadata.Info.c_str());
				}
			}
			else if (g_pState->Infrastructure->Lifecycle->GetCurrentPhase() == Phase::Director)
			{
				std::string requiredHash = g_pState->Domain->Timeline->GetAssociatedReplayHash();

				if (requiredHash.empty())
				{
					g_pUtil->Log.Append("[BlamOpenFile] WARNING: Director launched but no Timeline is loaded in memory.");
				}
				else if (currentFileHash != requiredHash)
				{
					g_pUtil->Log.Append("[BlamOpenFile] ERROR: Replay mismatch! Opened hash %s does not match Timeline hash %s."
						" Director phase cancelled to prevent crashes.",
						currentFileHash.substr(0, 8).c_str(), requiredHash.substr(0, 8).c_str());

					g_pThread->Main->UpdateToPhase(Phase::Default);
				}
				else
				{
					g_pUtil->Log.Append("[BlamOpenFile] INFO: Replay Hash matches Timeline, proceding.");
				}
			}
		}
	}
}

void BlamOpenFileHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = (void*)g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::BlamOpenFile);
	if (!functionAddress)
	{
		g_pUtil->Log.Append("[BlamOpenFile] ERROR: Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedBlamOpenFile, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pUtil->Log.Append("[BlamOpenFile] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pUtil->Log.Append(" [BlamOpenFile] ERROR: Failed to enable hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	g_pUtil->Log.Append("[BlamOpenFile] INFO: Hook installed.");
}

void BlamOpenFileHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pUtil->Log.Append("[BlamOpenFile] INFO: Hook uninstalled.");
}