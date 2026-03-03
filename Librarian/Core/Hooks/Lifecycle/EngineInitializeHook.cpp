#include "pch.h"
#include "Core/Hooks/Scanner.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "External/minhook/include/MinHook.h"

// This is the core initialization hook for the Blam! Engine environment.
// It serves as the "Orchestrator" for AutoTheater, responsible for selective hook 
// installation based on the current mod Lifecycle (Timeline vs. Director).
// Logic:
// 1. Guard Clause: Immediately exits if the session is not identified as 'TheaterMode'.
// 2. Timeline Phase: Installs diagnostic hooks (FLMH, GameEvents) to analyze and 
//    build the event database from the replay.
// 3. Director Phase: Installs execution hooks (Input, ButtonState) to allow the 
//    automated script to take control of the camera and playback.
// 4. Default Phase: Only maintains basic synchronization (Telemetry Timer).
void __fastcall EngineInitializeHook::HookedEngineInitialize(void)
{
	m_OriginalFunction();

	if (!g_pState->Theater.IsTheaterMode()) return;

	g_pHook->BlamOpenFile.Install();					// Gets the selected film path

	if (g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Timeline)
	{
		g_pUtil->Log.Append("[EngineInitialize] INFO: Timeline phase active.");
		
		g_pHook->FilmInitializeState.Install();			// Gets the FLMH data
		g_pHook->UpdateTelemetryTimer.Install();        // Gets the current ReplayTime
		g_pHook->BuildGameEvent.Install();				// Gets the GameEvents
		g_pHook->SpectatorHandleInput.Install();        // Gets the ReplayModule

		if (g_pState->Timeline.GetTimelineSize() > 0) g_pState->Timeline.ClearTimeline();
		if (g_pSystem->Timeline.GetLoggedEventsCount() > 0) g_pSystem->Timeline.SetLoggedEventsCount(0);
	}
	else if (g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Director)
	{
		g_pUtil->Log.Append("[EngineInitialize] INFO: Director phase active.");

		g_pHook->UpdateTelemetryTimer.Install();
		g_pHook->SpectatorHandleInput.Install();
		g_pHook->GetButtonState.Install();

		g_pState->Director.SetHooksReady(true);
	}
	else if (g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Default)
	{
		g_pUtil->Log.Append("[EngineInitialize] INFO: Default phase active.");

		g_pHook->UpdateTelemetryTimer.Install();
	}

	g_pSystem->Theater.InitializeReplaySpeed();
	g_pState->Lifecycle.SetEngineStatus({ EngineStatus::Running });
}

bool EngineInitializeHook::Install(bool silent)
{
	if (m_IsHookInstalled.load()) return true;

	void* functionAddress = (void*)Scanner::FindPattern(Signatures::EngineInitialize);
	if (!functionAddress)
	{
		if (!silent) g_pUtil->Log.Append("[EngineInitialize] ERROR: Failed to obtain the function address.");
		return false;
	}

	m_FunctionAddress.store(functionAddress);
	MH_RemoveHook(m_FunctionAddress.load());
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedEngineInitialize, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pUtil->Log.Append("[EngineInitialize] ERROR: Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		g_pUtil->Log.Append("[EngineInitialize] ERROR: Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	g_pUtil->Log.Append("[EngineInitialize] INFO: Hook installed.");
	return true;
}

void EngineInitializeHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pUtil->Log.Append("[EngineInitialize] INFO: Hook uninstalled.");
}

void* EngineInitializeHook::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}