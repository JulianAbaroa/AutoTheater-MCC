#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Core/Scanner/Scanner.h"
#include "Core/Systems/Director.h"
#include "Hooks/Lifecycle/GameEngineStart_Hook.h"
#include "Hooks/Lifecycle/EngineInitialize_Hook.h"
#include "External/minhook/include/MinHook.h"

#include "Hooks/Data/GetButtonState_Hook.h"
#include "Hooks/Data/SpectatorHandleInput_Hook.h"
#include "Hooks/Data/UpdateTelemetryTimer_Hook.h"
#include "Hooks/Data/UIBuildDynamicMessage_Hook.h"
#include "Hooks/MovReader/FilmInitializeState_Hook.h"
#include "Hooks/MovReader/BlamOpenFile_Hook.h"
#include <thread>

EngineInitialize_t original_EngineInitialize = nullptr;
std::atomic<bool> g_EngineInitialize_Hook_Installed = false;
void* g_EngineInitialize_Address = nullptr;

std::atomic<bool> g_EngineHooksReady{ false };

void __fastcall hkEngineInitialize(void)
{
	original_EngineInitialize();

	if (!g_IsTheaterMode) return;

	if (g_CurrentPhase == LibrarianPhase::BuildTimeline)
	{
		Logger::LogAppend("=== Build timeline ===");
		
		BlamOpenFile_Hook::Install();                   // Gets the selected film path
		FilmInitializeState_Hook::Install();            // Gets the FLMH data
		UpdateTelemetryTimer_Hook::Install();           // Gets the current ReplayTime
		UIBuildDynamicMessage_Hook::Install();          // Gets the GameEvents
		SpectatorHandleInput_Hook::Install();           // Gets the ReplayModule

		return;
	}
	else if (g_CurrentPhase == LibrarianPhase::ExecuteDirector)
	{
		Logger::LogAppend("=== Execute Director ===");

		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		UpdateTelemetryTimer_Hook::Install();
		SpectatorHandleInput_Hook::Install();
		GetButtonState_Hook::Install();

		g_EngineHooksReady.store(true);
	}
}

bool EngineInitialize_Hook::Install(bool silent)
{
	if (g_EngineInitialize_Hook_Installed.load()) return true;

	void* methodAddress = (void*)Scanner::FindPattern(Signatures::EngineInitialize);
	if (!methodAddress)
	{
		if (!silent) Logger::LogAppend("Failed to obtain the address of EngineInitialize()");
		return false;
	}

	g_EngineInitialize_Address = methodAddress;

	MH_RemoveHook(g_EngineInitialize_Address);

	MH_STATUS status = MH_CreateHook(g_EngineInitialize_Address, &hkEngineInitialize, reinterpret_cast<LPVOID*>(&original_EngineInitialize));
	if (status != MH_OK)
	{
		Logger::LogAppend(std::string("Failed to create EngineInitialize hook: " + std::to_string((int)status)).c_str());
		return false;
	}

	if (MH_EnableHook(g_EngineInitialize_Address) != MH_OK) {
		Logger::LogAppend("Failed to enable EngineInitialize hook");
		return false;
	}

	g_EngineInitialize_Hook_Installed.store(true);
	Logger::LogAppend("EngineInitialize hook installed");
	return true;
}

void EngineInitialize_Hook::Uninstall()
{
	if (!g_EngineInitialize_Hook_Installed.load()) return;

	MH_DisableHook(g_EngineInitialize_Address);
	MH_RemoveHook(g_EngineInitialize_Address);

	g_EngineInitialize_Hook_Installed.store(false);
	Logger::LogAppend("EngineInitialize hook uninstalled");
}