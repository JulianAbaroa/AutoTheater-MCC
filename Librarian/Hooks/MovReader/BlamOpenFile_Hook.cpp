#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Scanner/Scanner.h"
#include "Core/Systems/Theater.h"
#include "Hooks/MovReader/BlamOpenFile_Hook.h"
#include "External/minhook/include/MinHook.h"

BlamOpenFile_t original_BlamOpenFile = nullptr;
std::atomic<bool> g_BlamOpenFile_Hook_Installed;
void* g_BlamOpenFile_Address;

std::string g_FilmPath = "";


void hkBlam_OpenFile(
	long long fileContext,
	uint32_t accessFlags,
	uint32_t* translatedStatus
) {
	original_BlamOpenFile(fileContext, accessFlags, translatedStatus);

	const char* filePath = (const char*)((uintptr_t)fileContext + 0x8);

	if (filePath != nullptr && !IsBadReadPtr(filePath, 4)) {
		std::string pathStr(filePath);
		if (pathStr.find(".mov") != std::string::npos) {
			g_FilmPath = filePath;

			Logger::LogAppend((std::string("Film path: ") + filePath).c_str());

			Logger::LogAppend("=== Analyzing CHDR from Disk ===");

			std::ifstream file(pathStr, std::ios::binary);
			if (file.is_open()) {
				char buffer[0x400] = { 0 };
				file.read(buffer, sizeof(buffer));

				char author[17] = { 0 };
				memcpy(author, buffer + 0x88, 16);
				Logger::LogAppend((std::string("Recorded by: ") + author).c_str());

				wchar_t* wFullInfo = reinterpret_cast<wchar_t*>(buffer + 0x1C0);
				Logger::LogAppend((std::string("Info: ") + Formatting::WStringToString(wFullInfo)).c_str());

				file.close();
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