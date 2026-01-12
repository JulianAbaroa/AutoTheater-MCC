#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Scanner/Scanner.h"
#include "Core/Systems/Timeline.h"
#include "Hooks/MovReader/MovParser.h"
#include "Hooks/MovReader/FilmInitializeState_Hook.h"

FilmInitializeState_t original_FilmInitializeState = nullptr;
std::atomic<bool> g_FilmInitializeState_Hook_Installed;
void* g_FilmInitializeState_Address;

void hkFilm_InitializeState(
	uint64_t sessionContext,
	uint64_t headerBuffer
) {
	if (headerBuffer != 0)
	{
		Logger::LogAppend("=== Analyzing FLMH from Disk ===");
		
		std::stringstream ss;
		std::vector<int> missingIndices;

		char* mapName = reinterpret_cast<char*>(headerBuffer + 0x930);
		ss << "Base map: " << std::string(mapName);
		Logger::LogAppend(ss.str().c_str());

		Logger::LogAppend("Players present at joining:");

		for (uint8_t i = 0; i < 16; i++)
		{
			uintptr_t playerOffset = headerBuffer + 0xBD0 + (i * 0xA0);
			wchar_t* wName = reinterpret_cast<wchar_t*>(playerOffset);

			if (wName[0] != 0 && iswprint(wName[0]))
			{
				std::string finalName = Formatting::ToCompactAlpha(wName);

				wchar_t* wTag = reinterpret_cast<wchar_t*>(playerOffset + 0x44);
				std::string tag = Formatting::WStringToString(wTag);

				PlayerInfo info;
				info.Name = finalName;
				info.Tag = tag;
				info.Id = i;
				info.IsVictim = false;

				g_PlayerList[i] = info;

				ss.str("");
				ss << "Player [" << std::to_string(i) << "]: " 
					<< finalName << " [" << tag << "]";
				Logger::LogAppend(ss.str().c_str());
			}
			else
			{
				missingIndices.push_back(i);
			}
		}

		if (!missingIndices.empty())
		{
			ss.str("");
			ss << "WARNING: Slots [";

			for (size_t k = 0; k < missingIndices.size(); ++k)
			{
				ss << missingIndices[k] << (k < missingIndices.size() - 1 ? ", " : "");
			}

			ss << "] are empty in header";
			Logger::LogAppend(ss.str().c_str());
		}
	}

	original_FilmInitializeState(sessionContext, headerBuffer);

	if (!g_FilmPath.empty())
	{
		MovParser::GetHeaderSizes(g_FilmPath);
	}
}

void FilmInitializeState_Hook::Install()
{
	if (g_FilmInitializeState_Hook_Installed.load()) return;

	void* methodAddress = (void*)Scanner::FindPattern(Signatures::FilmInitializeState);
	if (!methodAddress)
	{
		Logger::LogAppend("Failed to obtain the address of FilmInitializeState()");
		return;
	}

	g_FilmInitializeState_Address = methodAddress;

	if (MH_CreateHook(g_FilmInitializeState_Address, &hkFilm_InitializeState, reinterpret_cast<LPVOID*>(&original_FilmInitializeState)) != MH_OK)
	{
		Logger::LogAppend("Failed to create FilmInitializeState hook");
		return;
	}

	if (MH_EnableHook(g_FilmInitializeState_Address) != MH_OK)
	{
		Logger::LogAppend("Failed to enalbe FilmInitializeState hook");
		return;
	}

	g_FilmInitializeState_Hook_Installed.store(true);
	Logger::LogAppend("FilmInitializeState hook installed");
}

void FilmInitializeState_Hook::Uninstall()
{
	if (!g_FilmInitializeState_Hook_Installed.load()) return;

	MH_DisableHook(g_FilmInitializeState_Address);
	MH_RemoveHook(g_FilmInitializeState_Address);

	g_FilmInitializeState_Hook_Installed.store(false);
	Logger::LogAppend("FilmInitializeState hook uninstalled");
}