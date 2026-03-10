#include "pch.h"
#include "Core/Utils/CoreUtil.h"
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
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/AudioSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Hooks/Lifecycle/DestroySubsystemsHook.h"
#include "External/minhook/include/MinHook.h"

// This function is triggered when the game engine shuts down the Halo: Reach simulation, 
// typically when returning to the MCC main menu.
// Purpose: Performs a cleanup of all AutoTheater interceptions.
// Logic:
// 1. Hook Removal: Uninstalls all active hooks to prevent the engine from executing 
//    detoured code in invalid memory contexts (Main Menu/Other Games).
// 2. Resource Reset: Invalidates pointers to engine-specific variables (Time, TimeScale) 
//    to avoid "dangling pointer" access in subsequent sessions.
// 3. Lifecycle Update: Sets the engine status to 'Destroyed' to signal all background 
//    systems (Director/Timeline) to stop processing immediately.
void __fastcall DestroySubsystemsHook::HookedDestroySubsystems(void)
{
	g_pHook->Data->BuildGameEvent->Uninstall();	
	g_pHook->Data->UpdateTelemetryTimer->Uninstall();
	g_pHook->Data->SpectatorHandleInput->Uninstall();
	g_pHook->Data->ReplayInitializeState->Uninstall();
	g_pHook->Input->GetButtonState->Uninstall();
	g_pHook->Data->BlamOpenFile->Uninstall();

	g_pState->Domain->Theater->SetTimePtr(nullptr);
	g_pState->Domain->Theater->SetTimeScalePtr(nullptr);

	if (g_pState->Infrastructure->FFmpeg->IsRecording()) g_pSystem->Infrastructure->FFmpeg->ForceStop();
	if (g_pState->Infrastructure->Audio->GetMasterInstance() != nullptr) g_pSystem->Infrastructure->Audio->Cleanup();

	g_pState->Infrastructure->Lifecycle->SetEngineStatus({ EngineStatus::Destroyed });

	m_OriginalFunction();

	g_pUtil->Log.Append("[DestroySubsystems] INFO: Game engine destroyed.");
}

bool DestroySubsystemsHook::Install(bool silent)
{
	if (m_IsHookInstalled.load()) return true;

	void* functionAddress = (void*)g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::DestroySubsystems);
	if (!functionAddress)
	{
		if (!silent) g_pUtil->Log.Append("[DestroySubsystems] ERROR: Failed to obtain the function address.");
		return false;
	}

	m_FunctionAddress.store(functionAddress);
	MH_RemoveHook(m_FunctionAddress.load());
	if (MH_CreateHook(m_FunctionAddress.load(), &HookedDestroySubsystems, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pUtil->Log.Append("[DestroySubsystems] ERROR: Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		g_pUtil->Log.Append("[DestroySubsystems] ERROR: Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	g_pUtil->Log.Append("[DestroySubsystems] INFO: Hook installed.");
	return true;
}

void DestroySubsystemsHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pUtil->Log.Append("[DestroySubsystems] INFO: Hook uninstalled.");
}

void* DestroySubsystemsHook::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}