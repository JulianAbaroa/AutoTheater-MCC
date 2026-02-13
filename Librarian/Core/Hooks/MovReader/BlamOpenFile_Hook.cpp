#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Common/AppCore.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include "Core/Threads/MainThread.h"
#include "Core/Common/PersistenceManager.h"
#include "Core/Hooks/MovReader/BlamOpenFile_Hook.h"
#include "External/minhook/include/MinHook.h"
#include <fstream>

BlamOpenFile_t original_BlamOpenFile = nullptr;
std::atomic<bool> g_BlamOpenFile_Hook_Installed;
void* g_BlamOpenFile_Address;

void hkBlam_OpenFile(
	long long fileContext,
	uint32_t accessFlags,
	uint32_t* translatedStatus
) {
	original_BlamOpenFile(fileContext, accessFlags, translatedStatus);

	const char* filePath = (const char*)((uintptr_t)fileContext + 0x8);

	if (filePath != nullptr && !IsBadReadPtr(filePath, 4)) 
	{
		std::string pathStr(filePath);
		if (pathStr.find(".mov") != std::string::npos) 
		{
			g_pState->Replay.SetCurrentFilmPath(filePath);

			std::ifstream file(pathStr, std::ios::binary);
			char author[17] = { 0 };
			wchar_t* wFullInfo{};

			if (file.is_open())
			{
				char buffer[0x400] = { 0 };
				file.read(buffer, sizeof(buffer));

				memcpy(author, buffer + 0x88, 16);

				wFullInfo = reinterpret_cast<wchar_t*>(buffer + 0x1C0);

				// TODO: Possibly author wstring to string needed
				g_pState->Replay.SetFilmMetadata({ author, Formatting::WStringToString(wFullInfo) });

				file.close();
			}

			if (g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Timeline)
			{
				Logger::LogAppend((std::string("Film path: ") + filePath).c_str());
				Logger::LogAppend("=== Analyzing CHDR from Disk ===");
				Logger::LogAppend((std::string("Recorded by: ") + author).c_str());

				if (wFullInfo != nullptr && wFullInfo[0] != L'\0')
				{
					Logger::LogAppend((std::string("Info: ") + Formatting::WStringToString(wFullInfo)).c_str());
				}
			}
			else if (g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Director)
			{
				std::string currentFileHash = g_pSystem->Replay.CalculateFileHash(filePath);
				std::string requiredHash = g_pState->Replay.GetActiveReplayHash();

				if (!requiredHash.empty() && currentFileHash != requiredHash)
				{
					Logger::LogAppend("WARNING: Replay mismatch! This timeline is not compatible with the opened film.");

					MainThread::UpdateToPhase(AutoTheaterPhase::Default);
				}
			}
		}
	}
}

void BlamOpenFile_Hook::Install()
{
	if (g_BlamOpenFile_Hook_Installed.load()) return;

	void* methodAddress = (void*)Scanner::FindPattern(Signatures::BlamOpenFile);
	if (!methodAddress)
	{
		Logger::LogAppend("Failed to obtain the address of BlamOpenFile()");
		return;
	}

	g_BlamOpenFile_Address = methodAddress;

	if (MH_CreateHook(g_BlamOpenFile_Address, &hkBlam_OpenFile, reinterpret_cast<LPVOID*>(&original_BlamOpenFile)) != MH_OK)
	{
		Logger::LogAppend("Failed to create BlamOpenFile hook");
		return;
	}

	if (MH_EnableHook(g_BlamOpenFile_Address) != MH_OK)
	{
		Logger::LogAppend("Failed to enalbe BlamOpenFile hook");
		return;
	}

	g_BlamOpenFile_Hook_Installed.store(true);
	Logger::LogAppend("BlamOpenFile hook installed");
}

void BlamOpenFile_Hook::Uninstall()
{
	if (!g_BlamOpenFile_Hook_Installed.load()) return;

	MH_DisableHook(g_BlamOpenFile_Address);
	MH_RemoveHook(g_BlamOpenFile_Address);

	g_BlamOpenFile_Hook_Installed.store(false);
	Logger::LogAppend("BlamOpenFile hook uninstalled");
}