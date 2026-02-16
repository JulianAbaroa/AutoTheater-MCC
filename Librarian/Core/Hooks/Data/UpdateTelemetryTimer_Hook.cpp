#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/AppCore.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include "Core/Hooks/Data/UpdateTelemetryTimer_Hook.h"
#include "External/minhook/include/MinHook.h"

UpdateTelemetryTimer_t original_UpdateTelemetryTimer = nullptr;
std::atomic<bool> g_UpdateTelemetryTimer_Hook_Installed;
void* g_UpdateTelemetryTimer_Address;

// This function updates the engine's internal telemetry timers and simulation clock.
// AutoTheater uses this hook as a high-frequency synchronization point to:
// 1. Dynamic Discovery: On the first execution, it performs a pattern scan to locate 
//    the 'ReplayTime' variable in memory using relative RVA offsets.
// 2. State Synchronization: Once found, it stores the pointer to the engine's 
//    absolute replay time, allowing the Director to sync camera cuts with the simulation.
// 3. Efficiency: By caching the pointer after the first match, it avoids the 
//    overhead of repeated signature scanning in subsequent frames.
void hkUpdateTelemetryTimer(
	uint64_t timerContext,
	float deltaTime
) {
	original_UpdateTelemetryTimer(timerContext, deltaTime);

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
			Logger::LogAppend("ReplayTime pointer found and stored.");
		}
		else
		{
			static bool loggedError = false;
			if (!loggedError)
			{
				Logger::LogAppend("Match not found for Signatures::TimeModifier");
				loggedError = true;
			}
		}
	}
}

void UpdateTelemetryTimer_Hook::Install()
{
	if (g_UpdateTelemetryTimer_Hook_Installed.load()) return;

	void* methodAddress = (void*)Scanner::FindPattern(Signatures::UpdateTelemetryTimer);

	if (!methodAddress)
	{
		Logger::LogAppend("Failed to obtain the address of UpdateTelemetryTimer()");
		return;
	}

	g_UpdateTelemetryTimer_Address = methodAddress;

	if (MH_CreateHook(g_UpdateTelemetryTimer_Address, &hkUpdateTelemetryTimer, reinterpret_cast<LPVOID*>(&original_UpdateTelemetryTimer)) != MH_OK)
	{
		Logger::LogAppend("Failed to create UpdateTelemetryTimer hook");
		return;
	}

	if (MH_EnableHook(g_UpdateTelemetryTimer_Address) != MH_OK)
	{
		Logger::LogAppend("Failed to enalbe UpdateTelemetryTimer hook");
		return;
	}

	g_UpdateTelemetryTimer_Hook_Installed.store(true);
	Logger::LogAppend("UpdateTelemetryTimer hook installed");
}

void UpdateTelemetryTimer_Hook::Uninstall()
{
	if (!g_UpdateTelemetryTimer_Hook_Installed.load()) return;

	MH_DisableHook(g_UpdateTelemetryTimer_Address);
	MH_RemoveHook(g_UpdateTelemetryTimer_Address);

	g_UpdateTelemetryTimer_Hook_Installed.store(false);
	Logger::LogAppend("UpdateTelemetryTimer hook uninstalled");
}