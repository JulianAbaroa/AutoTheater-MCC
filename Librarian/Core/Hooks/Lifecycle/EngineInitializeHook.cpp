#include "pch.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Data/CoreDataHook.h"
#include "Core/Hooks/Data/BlamOpenFileHook.h"
#include "Core/Hooks/Data/BuildGameEventHook.h"
#include "Core/Hooks/Data/ReplayInitializeStateHook.h"
#include "Core/Hooks/Data/SpectatorHandleInputHook.h"
#include "Core/Hooks/Data/UpdateTelemetryTimerHook.h"
#include "Core/Hooks/Input/CoreInputHook.h"
#include "Core/Hooks/Input/GetButtonStateHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Domain/Timeline/TimelineState.h"
#include "Core/States/Domain/Director/DirectorState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Timeline/TimelineSystem.h"
#include "Core/Systems/Domain/Theater/TheaterSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Hooks/Lifecycle/EngineInitializeHook.h"
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

	if (!g_pState->Domain->Theater->IsTheaterMode()) return;

	g_pHook->Data->BlamOpenFile->Install();					// Gets the selected replay path

	if (g_pState->Infrastructure->Lifecycle->GetCurrentPhase() == Phase::Timeline)
	{
		g_pSystem->Debug->Log("[EngineInitialize] INFO: Timeline phase active.");
		
		g_pHook->Data->ReplayInitializeState->Install();			// Gets the FLMH data
		g_pHook->Data->UpdateTelemetryTimer->Install();        // Gets the current ReplayTime
		g_pHook->Data->BuildGameEvent->Install();				// Gets the GameEvents
		g_pHook->Data->SpectatorHandleInput->Install();        // Gets the ReplayModule

		if (g_pState->Domain->Timeline->GetTimelineSize() > 0) g_pState->Domain->Timeline->ClearTimeline();
		if (g_pSystem->Domain->Timeline->GetLoggedEventsCount() > 0) g_pSystem->Domain->Timeline->SetLoggedEventsCount(0);
	}
	else if (g_pState->Infrastructure->Lifecycle->GetCurrentPhase() == Phase::Director)
	{
		g_pSystem->Debug->Log("[EngineInitialize] INFO: Director phase active.");

		g_pHook->Data->UpdateTelemetryTimer->Install();
		g_pHook->Data->SpectatorHandleInput->Install();
		g_pHook->Input->GetButtonState->Install();

		g_pState->Domain->Director->SetHooksReady(true);
	}
	else if (g_pState->Infrastructure->Lifecycle->GetCurrentPhase() == Phase::Default)
	{
		g_pSystem->Debug->Log("[EngineInitialize] INFO: Default phase active.");

		g_pHook->Data->UpdateTelemetryTimer->Install();
	}

	g_pSystem->Domain->Theater->InitializeReplaySpeed();
	g_pState->Infrastructure->Lifecycle->SetEngineStatus({ EngineStatus::Running });
}

bool EngineInitializeHook::Install(bool silent)
{
	if (m_IsHookInstalled.load()) return true;

	void* functionAddress = (void*)g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::EngineInitialize);
	if (!functionAddress)
	{
		if (!silent)  g_pSystem->Debug->Log("[EngineInitialize] ERROR: Failed to obtain the function address.");
		return false;
	}

	m_FunctionAddress.store(functionAddress);
	MH_RemoveHook(m_FunctionAddress.load());
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedEngineInitialize, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pSystem->Debug->Log("[EngineInitialize] ERROR: Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		g_pSystem->Debug->Log("[EngineInitialize] ERROR: Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[EngineInitialize] INFO: Hook installed.");
	return true;
}

void EngineInitializeHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[EngineInitialize] INFO: Hook uninstalled.");
}

void* EngineInitializeHook::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}