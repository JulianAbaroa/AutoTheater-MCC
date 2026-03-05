#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Hooks/Data/BlamOpenFileHook.h"
#include "External/minhook/include/MinHook.h"

// This function is a hook for the Blam! File System API, used for interacting with Windows I/O.
// It is triggered during engine initialization and state transitions to load maps,
// game variants, and resource packages.
// AutoTheater Interception: By hooking this call, the mod identifies the specific replay
// file (.mov) being loaded by the player. This allows AutoTheater to:
// 1. Scan and extract 'CHDR' (Film Metadata) from the disk.
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
			g_pState->Replay.SetPreviousReplayPath(g_pState->Replay.GetCurrentReplayPath());
			g_pState->Replay.SetCurrentReplayPath(filePath);

			TheaterReplay replay = g_pSystem->Replay.ScanReplay(filePath);

			if (g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Timeline)
			{
				g_pUtil->Log.Append("[BlamOpenFile] INFO: Film path: %s", filePath);
				g_pUtil->Log.Append("[BlamOpenFile] INFO: Analyzing CHDR from Disk.");
				g_pUtil->Log.Append("[BlamOpenFile] INFO: Recorded by: %s", replay.FilmMetadata.Author);

				if (!replay.FilmMetadata.Info.empty())
				{
					g_pUtil->Log.Append("[BlamOpenFile] INFO: %s", replay.FilmMetadata.Info.c_str());
				}
			}
			else if (g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Director &&
				filePath != g_pState->Replay.GetPreviousReplayPath())
			{
				std::string currentHash = g_pSystem->Replay.CalculateFileHash(filePath);
				std::string requiredHash = g_pSystem->Replay.CalculateFileHash(g_pState->Replay.GetPreviousReplayPath());
				
				if (!requiredHash.empty() && currentHash == requiredHash)
				{
					g_pUtil->Log.Append("[BlamOpenFile] WARNING: Replay mismatch! This timeline is not compatible with the opened replay.");
					g_pThread->Main.UpdateToPhase(AutoTheaterPhase::Default);
				}
			}
		}
	}
}

void BlamOpenFileHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = (void*)g_pSystem->Scanner.FindPattern(Signatures::BlamOpenFile);
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