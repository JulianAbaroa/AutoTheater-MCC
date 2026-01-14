#include "pch.h"
#include "Utils/Logger.h"
#include "Core/DllMain.h"
#include "Core/Scanner/Scanner.h"
#include "Hooks/Lifecycle/GameEngineStart_Hook.h"

bool g_IsTheaterMode = false;

GameEngineStart_t original_GameEngineStart = nullptr;
std::atomic<bool> g_GameEngineStart_Hook_Installed = false;
void* g_GameEngineStart_Address = nullptr;

void __fastcall hkGameEngineStart(uint64_t param_1, uint64_t param_2, uint64_t* param_3)
{
	original_GameEngineStart(param_1, param_2, param_3);

	const uint8_t theaterSignature[] = {
		0x08, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00
	};

	if (memcmp(param_3, theaterSignature, 16) == 0)
	{
		Logger::LogAppend("Theater mode detected, proceding...");
		g_IsTheaterMode = true;
	}
	else
	{
		Logger::LogAppend("Theater mode wasn't detected, aborting...");
		g_IsTheaterMode = false;
	}
}

bool GameEngineStart_Hook::Install()
{
	if (g_GameEngineStart_Hook_Installed.load()) return true;

	void* methodAddress = (void*)Scanner::FindPattern(Signatures::GameEngineStart);
	if (!methodAddress)
	{
		Logger::LogAppend("Failed to obtain the address of GameEngineStart()");
		return false;
	}

	g_GameEngineStart_Address = methodAddress;

	if (MH_CreateHook(g_GameEngineStart_Address, &hkGameEngineStart, reinterpret_cast<LPVOID*>(&original_GameEngineStart)) != MH_OK)
	{
		Logger::LogAppend("Failed to create GameEngineStart hook");
		return false;
	}

	if (MH_EnableHook(g_GameEngineStart_Address) != MH_OK)
	{
		Logger::LogAppend("Failed to enable GameEngineStart hook");
		return false;
	}

	g_GameEngineStart_Hook_Installed.store(true);
	Logger::LogAppend("GameEngineStart hook installed");
	return true;
}

void GameEngineStart_Hook::Uninstall()
{
	if (!g_GameEngineStart_Hook_Installed.load()) return;

	MH_DisableHook(g_GameEngineStart_Address);
	MH_RemoveHook(g_GameEngineStart_Address);

	g_GameEngineStart_Hook_Installed.store(false);
	Logger::LogAppend("GameEngineStart hook uninstalled");
}