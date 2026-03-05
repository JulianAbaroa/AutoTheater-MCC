#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
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
	g_pHook->BuildGameEvent.Uninstall();	
	g_pHook->UpdateTelemetryTimer.Uninstall();
	g_pHook->SpectatorHandleInput.Uninstall();
	g_pHook->FilmInitializeState.Uninstall();
	g_pHook->GetButtonState.Uninstall();
	g_pHook->BlamOpenFile.Uninstall();

	g_pState->Theater.SetTimePtr(nullptr);
	g_pState->Theater.SetTimeScalePtr(nullptr);

	if (g_pState->FFmpeg.IsRecording()) g_pSystem->FFmpeg.ForceStop();

	g_pState->Lifecycle.SetEngineStatus({ EngineStatus::Destroyed });

	m_OriginalFunction();

	g_pUtil->Log.Append("[DestroySubsystems] INFO: Game engine destroyed.");
}

bool DestroySubsystemsHook::Install(bool silent)
{
	if (m_IsHookInstalled.load()) return true;

	void* functionAddress = (void*)g_pSystem->Scanner.FindPattern(Signatures::DestroySubsystems);
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