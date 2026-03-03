#include "pch.h"
#include "Core/Hooks/Scanner.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Hooks/DAta/UpdateTelemetryTimerHook.h"
#include "External/minhook/include/MinHook.h"

// This function updates the engine's internal telemetry timers and simulation clock.
// AutoTheater uses this hook as a high-frequency synchronization point to:
// 1. Dynamic Discovery: On the first execution, it performs a pattern scan to locate 
//    the 'ReplayTime' variable in memory using relative RVA offsets.
// 2. State Synchronization: Once found, it stores the pointer to the engine's 
//    absolute replay time, allowing the Director to sync camera cuts with the simulation.
// 3. Efficiency: By caching the pointer after the first match, it avoids the 
//    overhead of repeated signature scanning in subsequent frames.
void UpdateTelemetryTimerHook::HookedUpdateTelemetryTimer(uint64_t timerContext, float deltaTime)
{
	m_OriginalFunction(timerContext, deltaTime);

	float* currentPtr = g_pState->Theater.GetTimePtr();

	if (currentPtr == nullptr)
	{
		uintptr_t match = Scanner::FindPattern(Signatures::TimeModifier);
		if (match)
		{
			int32_t relativeOffset = *(int32_t*)(match + 4);
			uintptr_t replayTimeAddr = (match + 8) + relativeOffset;
			uintptr_t finalAddr = (replayTimeAddr - 0xC);

			g_pState->Theater.SetTimePtr(reinterpret_cast<float*>(finalAddr));
			g_pUtil->Log.Append("[UpdateTelemetryTimer] INFO: ReplayTime pointer found and stored.");
		}
		else
		{
			static bool loggedError = false;
			if (!loggedError)
			{
				g_pUtil->Log.Append("[UpdateTelemetryTimer] ERROR: Match not found for Signatures::TimeModifier");
				loggedError = true;
			}
		}
	}
}

void UpdateTelemetryTimerHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = (void*)Scanner::FindPattern(Signatures::UpdateTelemetryTimer);
	if (!functionAddress)
	{
		g_pUtil->Log.Append("[UpdateTelemetryTimer] ERROR: Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedUpdateTelemetryTimer, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pUtil->Log.Append("[UpdateTelemetryTimer] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pUtil->Log.Append("[UpdateTelemetryTimer] ERROR: Failed to enable the hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	g_pUtil->Log.Append("[UpdateTelemetryTimer] INFO: Hook installed.");
}

void UpdateTelemetryTimerHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pUtil->Log.Append("[UpdateTelemetryTimer] INFO: Hook uninstalled.");
}