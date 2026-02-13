#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Common/AppCore.h"
#include "Core/Systems/Domain/TheaterSystem.h"
#include "Core/Hooks/Data/Telemetry/Telemetry.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include <sstream>

void TheaterSystem::Update()
{
	if (!g_pState->Theater.IsTheaterMode()) return;

	UpdateRealTimeScale();

	if (g_pState->Theater.GetReplayModule())
	{
		TryGetSpectatedPlayerIndex(g_pState->Theater.GetReplayModule());
	}
}

void TheaterSystem::SetReplaySpeed(float speed)
{
	speed = std::clamp(speed, 0.0f, 32.0f);
	float* pTimeScale = g_pState->Theater.GetTimeScalePtr();

	if (!pTimeScale)
	{
		uintptr_t match = Scanner::FindPattern(Signatures::TimeScaleModifier);
		if (match)
		{
			int32_t relativeOffset = *(int32_t*)(match + 4);
			uintptr_t timeScaleAddr = (match + 8) + relativeOffset;
			pTimeScale = reinterpret_cast<float*>(timeScaleAddr);
			g_pState->Theater.SetTimeScalePtr(pTimeScale);
		}
		else
		{
			Logger::LogAppend("No match found for Signatures::TimeScaleModifier");
		}
	}

	if (pTimeScale) *pTimeScale = speed;
}

void TheaterSystem::RefreshPlayerList()
{
	uintptr_t playerTable = Telemetry::GetTelemetryPlayerTable();
	uintptr_t objectTable = Telemetry::GetTelemetryObjectTable();
	if (!playerTable || !objectTable) return;

	//LogTables(playerTable, objectTable);

	std::vector<PlayerInfo> nextPlayerList;
	nextPlayerList.resize(16);

	for (uint8_t i = 0; i < 16; i++)
	{
		PlayerInfo newPlayer = { 0 };

		if (RawReadSinglePlayer(playerTable, objectTable, i, newPlayer))
		{
			newPlayer.Name = Formatting::WStringToString(newPlayer.RawPlayer.Name);
			newPlayer.Tag = Formatting::WStringToString(newPlayer.RawPlayer.Tag);
			newPlayer.Id = i;

			nextPlayerList[i] = newPlayer;
		}
		else
		{
			nextPlayerList[i] = PlayerInfo();
			Logger::LogAppend("RawReadSinglePlayer failed");
		}
	}

	g_pState->Theater.SetPlayerList(nextPlayerList);
}

bool TheaterSystem::TryGetSpectatedPlayerIndex(uint64_t pReplayModule)
{
	__try
	{
		uint8_t currentPlayer = *(uint8_t*)(pReplayModule + 0x184);

		if (currentPlayer >= 0 && currentPlayer < 16)
		{
			g_pState->Theater.SetSpectatedPlayerIndex(currentPlayer);
			return true;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	return false;
}

bool TheaterSystem::TryGetPlayerName(uint8_t slotID, wchar_t* outName, size_t maxChars) {
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


void TheaterSystem::LogTables(uintptr_t playerTable, uintptr_t objectTable)
{
	static bool isFirst = true;

	if (isFirst)
	{
		char buf[512];
		sprintf_s(buf, "DEBUG: PlayerTable: %llx | ObjectTable: %llx", playerTable, objectTable);
		Logger::LogAppend(buf);
		isFirst = false;
	}
}

void TheaterSystem::UpdateRealTimeScale()
{
	float* pTime = g_pState->Theater.GetTimePtr();
	if (!pTime) return;

	auto now = std::chrono::steady_clock::now();
	double currentWallClock = std::chrono::duration<double>(now.time_since_epoch()).count();
	float currentReplayTime = *pTime;

	double elapsedWall = currentWallClock - g_pSystem->Theater.GetAnchorSystemTime();

	if (elapsedWall >= 0.1f)
	{
		float deltaReplay = currentReplayTime - g_pSystem->Theater.GetAnchorReplayTime();

		if (elapsedWall > 0)
		{
			float actualSpeed = static_cast<float>(deltaReplay / elapsedWall);

			float lastScale = g_pSystem->Theater.GetRealTimeScale();
			float smoothed = (lastScale * 0.7f) + (actualSpeed * 0.3f);

			if (smoothed < 0.01f) smoothed = 0.0f;

			g_pSystem->Theater.SetRealTimeScale(smoothed);
		}

		g_pSystem->Theater.SetAnchorSystemTime(currentWallClock);
		g_pSystem->Theater.SetAnchorReplayTime(currentReplayTime);
	}
}

// TODO: Currently is not copying the Rotation and LookVector from memory.
bool TheaterSystem::RawReadSinglePlayer(
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
		rawPlayer.hCurrentBiped = *(uint32_t*)(playerAddress + 0x28);
		rawPlayer.hPreviousBiped = *(uint32_t*)(playerAddress + 0x2C);
		rawPlayer.hBiped = *(uint32_t*)(playerAddress + 0x34);
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


float TheaterSystem::GetRealTimeScale() const
{
	return m_RealTimeScale.load();
}

double TheaterSystem::GetAnchorSystemTime() const
{
	return m_AnchorSystemTime.load();
}

float TheaterSystem::GetAnchorReplayTime() const
{
	return m_AnchorReplayTime.load();
}

void TheaterSystem::SetRealTimeScale(float realTimeScale)
{
	m_RealTimeScale.store(realTimeScale);
}

void TheaterSystem::SetAnchorSystemTime(double anchorSystemTime)
{
	m_AnchorSystemTime.store(anchorSystemTime);
}

void TheaterSystem::SetAnchorReplayTime(float anchorReplayTime)
{
	m_AnchorReplayTime.store(anchorReplayTime);
}