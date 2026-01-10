#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Scanner/Scanner.h"
#include "Core/Systems/Theater.h"
#include "Hooks/Telemetry/Telemetry.h"

std::vector<PlayerInfo> g_PlayerList(16);
uint8_t g_FollowedPlayerIdx = 255;
uintptr_t g_pReplayModule = 0;
std::string g_FilmPath = "";

float* g_pReplayTimeScale = nullptr;
float* g_pReplayTime = nullptr;

void Theater::SetReplaySpeed(float speed)
{
	if (g_pReplayTimeScale == nullptr)
	{
		uintptr_t match = Scanner::FindPattern(Sig_TimeScaleModifier);

		if (match)
		{
			int32_t relativeOffset = *(int32_t*)(match + 4);
			uintptr_t timeScaleAddr = (match + 8) + relativeOffset;
			g_pReplayTimeScale = reinterpret_cast<float*>(timeScaleAddr);
			*g_pReplayTimeScale = speed;
		}
		else
		{
			Logger::LogAppend("No match found for Sig_TimeScaleModifier");
		}
	}
	else
	{
		*g_pReplayTimeScale = speed;
	}
}

static bool RawReadSinglePlayer(
	uintptr_t playerTable, 
	uintptr_t objectTable, 
	uint8_t index, 
	PlayerInfo& outInfo
) {
	uintptr_t playerAddress = playerTable + (index * 0x490);
	RawPlayer& rawPlayer = outInfo.RawPlayer;
	int checkpoint = 0;

	__try
	{
		if (playerAddress < 0x10000) return false;

		checkpoint = 1;
		rawPlayer.SlotID = *(uint32_t*)(playerAddress + 0x0);
		rawPlayer.BipedHandle = *(uint32_t*)(playerAddress + 0x28);
		memcpy(rawPlayer.Position, (void*)(playerAddress + 0x38), 12);
		memcpy(rawPlayer.Name, (void*)(playerAddress + 0xB0), 56);
		memcpy(rawPlayer.Tag, (void*)(playerAddress + 0xF4), 16);

		checkpoint = 2;
		uint32_t handles[3]{};
		handles[0] = *(uint32_t*)(playerAddress + 0x5C); 
		handles[1] = *(uint32_t*)(playerAddress + 0x60);
		handles[2] = *(uint32_t*)(playerAddress + 0x6C);

		rawPlayer.hPrimaryWeapon = handles[0];
		rawPlayer.hSecondaryWeapon = handles[1];
		rawPlayer.hObjective = handles[2];

		HANDLE hProc = GetCurrentProcess();

		for (int i = 0; i < 3; i++)
		{
			if (handles[i] == 0 || handles[i] == 0xFFFFFFFF) continue;

			checkpoint = 3;
			uint32_t objIndex = handles[i] & 0xFFFF;
			uintptr_t entryBase = objectTable + (objIndex * 0x18);

			uintptr_t entryData[4] = { 0 };
			SIZE_T br = 0;

			if (ReadProcessMemory(hProc, (LPCVOID)entryBase, &entryData, sizeof(entryData), &br))
			{
				uintptr_t weaponPtr = 0;
				for (int j = 0; j < 4; j++) {
					if (entryData[j] > 0x700000000000) {
						weaponPtr = entryData[j];
						break;
					}
				}

				if (weaponPtr > 0x10000) {
					checkpoint = 4;
					RawWeapon tempW = { 0 };
					if (ReadProcessMemory(hProc, (LPCVOID)weaponPtr, &tempW, sizeof(RawWeapon), &br)) {
						outInfo.Weapons.push_back(tempW);
					}
				}
			}
		}
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		char buf[128];
		sprintf_s(buf, "CRASH en RawRead: Idx %d | Checkpoint %d | Addr: %llx", index, checkpoint, playerAddress);
		Logger::LogAppend(buf);
		return false;
	}

	return false;
}

void Theater::RebuildPlayerListFromMemory()
{
	uintptr_t playerTable = Telemetry::GetTelemetryPlayerTable(); 
	uintptr_t objectTable = Telemetry::GetTelemetryObjectTable();
	if (!playerTable || !objectTable) return;

	static bool isFirst = true;

	if (isFirst)
	{
		char buf[512];
		sprintf_s(buf, "DEBUG: PlayerTable: %llx | ObjectTable: %llx", playerTable, objectTable);
		Logger::LogAppend(buf);
		isFirst = false;
	}

	for (uint8_t i = 0; i < 16; i++)	
	{
		PlayerInfo newPlayer = { 0 };

		if (RawReadSinglePlayer(playerTable, objectTable, i, newPlayer))
		{
			newPlayer.Name = Formatting::WStringToString(newPlayer.RawPlayer.Name);
			newPlayer.Tag = Formatting::WStringToString(newPlayer.RawPlayer.Tag);
			newPlayer.Id = i;
			newPlayer.IsVictim = false;

			g_PlayerList[i] = newPlayer;
		}
		else
		{
			g_PlayerList[i] = PlayerInfo();
			Logger::LogAppend("RawReadSinglePlayer failed");
		}
	}
}

bool Theater::TryGetFollowedPlayerIdx(uint64_t pReplayModule)
{
	__try
	{
		uint8_t currentPlayer = *(uint8_t*)(pReplayModule + 0x184);

		if (currentPlayer >= 0 && currentPlayer < 16)
		{
			g_FollowedPlayerIdx = currentPlayer;
			return true;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) 
	{
		return false;
	}

	return false;
}

bool Theater::TryGetPlayerName(uint8_t slotID, wchar_t* outName, size_t maxChars) {
	__try {
		uintptr_t playerTable = Telemetry::GetTelemetryPlayerTable();
		if (!playerTable) return false;

		uintptr_t playerAddress = playerTable + (slotID * 0x490);
		wchar_t* pwName = (wchar_t*)(playerAddress + 0xB0);

		if (pwName && pwName[0] != L'\0') {
			wcscpy_s(outName, maxChars, pwName);
			return true;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}

	return false;
}