#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Core/Scanner/Scanner.h"
#include "Core/Systems/Theater.h"
#include "Hooks/Data/UpdateTelemetryTimer_Hook.h"

UpdateTelemetryTimer_t original_UpdateTelemetryTimer = nullptr;
std::atomic<bool> g_UpdateTelemetryTimer_Hook_Installed;
void* g_UpdateTelemetryTimer_Address;

void hkUpdateTelemetryTimer(
	uint64_t timerContext,
	float deltaTime
) {
	original_UpdateTelemetryTimer(timerContext, deltaTime);

	if (g_pReplayTime == nullptr)
	{
		uintptr_t match = Scanner::FindPattern(Sig_TimeModifier);

		if (match)
		{
			int32_t relativeOffset = *(int32_t*)(match + 4);
			uintptr_t replayTimeAddr = (match + 8) + relativeOffset;
			uintptr_t finalAddr = (replayTimeAddr - 0xC);
			g_pReplayTime = reinterpret_cast<float*>(finalAddr);
		}
		else
		{
			Logger::LogAppend("Match not found for Sig_TimeModifier");
		}
	}
}

void UpdateTelemetryTimer_Hook::Install()
{
	if (g_UpdateTelemetryTimer_Hook_Installed.load()) return;

	void* methodAddress = (void*)Scanner::FindPattern(Sig_UpdateTelemetryTimer);

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

	HMODULE hMod = GetModuleHandle(L"haloreach.dll");
	if (hMod != nullptr && g_UpdateTelemetryTimer_Address != 0)
	{
		MH_DisableHook(g_UpdateTelemetryTimer_Address);
		MH_RemoveHook(g_UpdateTelemetryTimer_Address);
		Logger::LogAppend("UpdateTelemetryTimer hook uninstalled safely");
	}
	else
	{
		Logger::LogAppend("UpdateTelemetryTimer hook skipped uninstall (module already gone)");
	}

	g_UpdateTelemetryTimer_Hook_Installed.store(false);
}