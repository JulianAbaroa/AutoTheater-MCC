#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/Hooks/Memory/PlayersTableHook.h"
#include "Core/Hooks/Memory/ObjectsTableHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Theater/TheaterSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"

void TheaterSystem::Update()
{
	if (!g_pState->Domain->Theater->IsTheaterMode()) return;

	this->UpdateRealTimeScale();

	if (g_pState->Domain->Theater->GetReplayModule())
	{
		TryGetSpectatedPlayerIndex(g_pState->Domain->Theater->GetReplayModule());
	}
}


void TheaterSystem::InitializeReplaySpeed()
{
	if (m_IsReplaySpeedInitialized.load()) return;

	float* pTimeScale = g_pState->Domain->Theater->GetTimeScalePtr();
	if (!pTimeScale)
	{
		g_pSystem->Domain->Theater->SetReplaySpeed(1.0f);
	}
}

void TheaterSystem::SetReplaySpeed(float speed)
{
	speed = std::clamp(speed, 0.0f, 32.0f);
	float* pTimeScale = g_pState->Domain->Theater->GetTimeScalePtr();

	if (!pTimeScale)
	{
		uintptr_t match = g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::TimeScaleModifier);
		if (match)
		{
			int32_t relativeOffset = *(int32_t*)(match + 4);
			uintptr_t timeScaleAddr = (match + 8) + relativeOffset;
			pTimeScale = reinterpret_cast<float*>(timeScaleAddr);
			g_pState->Domain->Theater->SetTimeScalePtr(pTimeScale);
		}
		else
		{
			g_pUtil->Log.Append("[TheaterSystem] ERROR: No match found for Signatures::TimeScaleModifier");
		}
	}

	if (pTimeScale) *pTimeScale = speed;
}

void TheaterSystem::RefreshPlayerList()
{
	uintptr_t playerTable = g_pHook->Memory->PlayersTable->GetPlayersTable();
	uintptr_t objectTable = g_pHook->Memory->ObjectsTable->GetObjectsTable();
	if (!playerTable || !objectTable) return;

	//LogTables(playerTable, objectTable);

	std::vector<PlayerInfo> nextPlayerList;
	nextPlayerList.resize(16);

	for (uint8_t i = 0; i < 16; i++)
	{
		PlayerInfo newPlayer = { 0 };

		if (RawReadSinglePlayer(playerTable, objectTable, i, newPlayer))
		{
			newPlayer.Name = g_pUtil->Format.WStringToString(newPlayer.RawPlayer.Name);
			newPlayer.Tag = g_pUtil->Format.WStringToString(newPlayer.RawPlayer.Tag);
			newPlayer.Id = i;

			nextPlayerList[i] = newPlayer;
		}
		else
		{
			nextPlayerList[i] = PlayerInfo();
			g_pUtil->Log.Append("[TheaterSystem] ERROR: RawReadSinglePlayer failed");
		}
	}

	g_pState->Domain->Theater->SetPlayerList(nextPlayerList);
}

bool TheaterSystem::TryGetSpectatedPlayerIndex(uint64_t pReplayModule)
{
	__try
	{
		uint8_t currentPlayer = *(uint8_t*)(pReplayModule + 0x184);

		if (currentPlayer >= 0 && currentPlayer < 16)
		{
			g_pState->Domain->Theater->SetSpectatedPlayerIndex(currentPlayer);
			return true;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	return false;
}

bool TheaterSystem::TryGetPlayerName(uint8_t slotID, wchar_t* outName, size_t maxChars) 
{
	__try 
	{
		uintptr_t playerTable = g_pHook->Memory->PlayersTable->GetPlayersTable();
		if (!playerTable) return false;

		uintptr_t playerAddress = playerTable + (slotID * 0x490);
		wchar_t* pwName = (wchar_t*)(playerAddress + 0xB0);

		if (pwName && pwName[0] != L'\0') 
		{
			wcscpy_s(outName, maxChars, pwName);
			return true;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) 
	{
		return false;
	}

	return false;
}


void TheaterSystem::LogTables(uintptr_t playerTable, uintptr_t objectTable)
{
	static bool isFirst = true;

	if (isFirst)
	{
		g_pUtil->Log.Append("[TheaterSystem] DEBUG: PlayerTable: %llx | ObjectTable: %llx", playerTable, objectTable);
		isFirst = false;
	}
}

void TheaterSystem::UpdateRealTimeScale()
{
	float* pTime = g_pState->Domain->Theater->GetTimePtr();
	if (!pTime) return;

	auto now = std::chrono::steady_clock::now();
	double currentWallClock = std::chrono::duration<double>(now.time_since_epoch()).count();
	float currentReplayTime = *pTime;

	double elapsedWall = currentWallClock - g_pSystem->Domain->Theater->GetAnchorSystemTime();

	if (elapsedWall >= 0.1f)
	{
		float deltaReplay = currentReplayTime - g_pSystem->Domain->Theater->GetAnchorReplayTime();

		if (elapsedWall > 0)
		{
			float actualSpeed = static_cast<float>(deltaReplay / elapsedWall);

			float lastScale = g_pSystem->Domain->Theater->GetRealTimeScale();
			float smoothed = (lastScale * 0.7f) + (actualSpeed * 0.3f);

			if (smoothed < 0.01f) smoothed = 0.0f;

			g_pSystem->Domain->Theater->SetRealTimeScale(smoothed);
		}

		g_pSystem->Domain->Theater->SetAnchorSystemTime(currentWallClock);
		g_pSystem->Domain->Theater->SetAnchorReplayTime(currentReplayTime);
	}
}

// TODO: Currently is not copying the Rotation and LookVector from memory.
// Reads and parses data for a single player by resolving their handles through the Object Table.
// 1. Data Access: Uses the 'playerTable' and 'index' to calculate the player's 0x490-sized data block.
// 2. Handle Resolution: Extracts the low 16 bits (0xFFFF) from object handles to use as an index 
//    within the Global Object Table.
// 3. Pointer Indirection: Each 24-byte (0x18) entry in the Object Table is scanned for a valid 
//    64-bit memory address (pointing to the actual Weapon, Biped, or Objective entity).
// 4. Safety: Uses Structured Exception Handling (__try/__except) and manual pointer validation 
//    to prevent crashes during memory scraping.
bool TheaterSystem::RawReadSinglePlayer(
	uintptr_t playerTable, 
	uintptr_t objectTable, 
	uint8_t index, 
	PlayerInfo& outInfo) 
{
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
				for (int j = 0; j < 4; j++) 
				{
					if (entryData[j] > 0x700000000000) 
					{
						weaponPtr = entryData[j];
						break;
					}
				}

				if (weaponPtr > 0x10000) 
				{
					checkpoint = 4;
					RawWeapon tempW = { 0 };
					if (ReadProcessMemory(hProc, (LPCVOID)weaponPtr, &tempW, sizeof(RawWeapon), &br)) 
					{
						outInfo.Weapons.push_back(tempW);
					}
				}
			}
		}

		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		g_pUtil->Log.Append("[TheaterSystem] ERROR: In RawRead Idx %d | Checkpoint %d | Addr: %016llx",
			index, checkpoint, playerAddress);
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