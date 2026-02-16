#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/AppCore.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include "Core/Hooks/Lifecycle/GameEngineStart_Hook.h"
#include "External/minhook/include/MinHook.h"

GameEngineStart_t original_GameEngineStart = nullptr;
std::atomic<bool> g_GameEngineStart_Hook_Installed = false;
void* g_GameEngineStart_Address = nullptr;

// This is the entry point for the Game Engine's primary initialization loop.
// It is triggered whenever a new simulation state is being prepared.
// Detection Logic: AutoTheater inspects 'param_3' for a specific 16-byte session 
// signature (CGB Theater Signature). This signature uniquely identifies if the 
// current engine state is a Theater replay session.
// Purpose: This prevents the mod from activating during normal gameplay (Campaign/MP), 
// ensuring that AutoTheater's director and hooks only engage when a valid 
// replay context is detected.
void __fastcall hkGameEngineStart(uint64_t param_1, uint64_t param_2, uint64_t* param_3)
{
	original_GameEngineStart(param_1, param_2, param_3);

	const uint8_t theaterSignature[] = {
		0x08, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00
	};

	if (memcmp(param_3, theaterSignature, 16) == 0)
	{
		Logger::LogAppend("CGB Theater detected, proceding...");
		g_pState->Theater.SetTheaterMode(true);
	}
	else
	{
		Logger::LogAppend("CGB Theater wasn't detected, aborting...");
		g_pState->Theater.SetTheaterMode(false);
	}
}

bool GameEngineStart_Hook::Install(bool silent)
{
	if (g_GameEngineStart_Hook_Installed.load()) return true;

	void* methodAddress = (void*)Scanner::FindPattern(Signatures::GameEngineStart);
	if (!methodAddress)
	{
		if (!silent) Logger::LogAppend("Failed to obtain the address of GameEngineStart()");
		return false;
	}

	g_GameEngineStart_Address = methodAddress;

	MH_RemoveHook(g_GameEngineStart_Address);

	MH_STATUS status = MH_CreateHook(g_GameEngineStart_Address, &hkGameEngineStart, reinterpret_cast<LPVOID*>(&original_GameEngineStart));
	if (status != MH_OK)
	{
		Logger::LogAppend(std::string("Failed to create GameEngineStart hook: " + std::to_string((int)status)).c_str());
		return false;
	}

	if (MH_EnableHook(g_GameEngineStart_Address) != MH_OK) {
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