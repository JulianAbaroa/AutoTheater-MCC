#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/AppCore.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include "Core/Hooks/Data/GetButtonState_Hook.h"
#include "Core/Hooks/Data/SpectatorHandleInput_Hook.h"
#include "Core/Hooks/Data/UpdateTelemetryTimer_Hook.h"
#include "Core/Hooks/Data/UIBuildDynamicMessage_Hook.h"
#include "Core/Hooks/MovReader/FilmInitializeState_Hook.h"
#include "Core/Hooks/MovReader/BlamOpenFile_Hook.h"
#include "Core/Hooks/Lifecycle/EngineInitialize_Hook.h"
#include "External/minhook/include/MinHook.h"
#include <chrono>

using namespace std::chrono_literals;

EngineInitialize_t original_EngineInitialize = nullptr;
std::atomic<bool> g_EngineInitialize_Hook_Installed = false;
void* g_EngineInitialize_Address = nullptr;

void __fastcall hkEngineInitialize(void)
{
	original_EngineInitialize();

	if (!g_pState->Theater.IsTheaterMode()) return;

	BlamOpenFile_Hook::Install();						// Gets the selected film path

	if (g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Timeline)
	{
		Logger::LogAppend("[Timeline Phase]");
		
		FilmInitializeState_Hook::Install();            // Gets the FLMH data
		UpdateTelemetryTimer_Hook::Install();           // Gets the current ReplayTime
		UIBuildDynamicMessage_Hook::Install();          // Gets the GameEvents
		SpectatorHandleInput_Hook::Install();           // Gets the ReplayModule

		if (g_pState->Timeline.GetTimelineSize() > 0) g_pState->Timeline.ClearTimeline();
		if (g_pSystem->Timeline.GetLoggedEventsCount() > 0) g_pSystem->Timeline.SetLoggedEventsCount(0);
	}
	else if (g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Director)
	{
		Logger::LogAppend("[Director Phase]");

		UpdateTelemetryTimer_Hook::Install();
		SpectatorHandleInput_Hook::Install();
		GetButtonState_Hook::Install();

		g_pState->Director.SetHooksReady(true);
	}
	else if (g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Default)
	{
		Logger::LogAppend("[Default Phase]");

		UpdateTelemetryTimer_Hook::Install();
	}

	g_pState->Lifecycle.SetEngineStatus({ EngineStatus::Running });
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