#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include "Core/Common/AppCore.h"
#include "Core/Hooks/Data/GetButtonState_Hook.h"
#include "Core/Hooks/Data/SpectatorHandleInput_Hook.h"
#include "Core/Hooks/Data/UpdateTelemetryTimer_Hook.h"
#include "Core/Hooks/Data/UIBuildDynamicMessage_Hook.h"
#include "Core/Hooks/MovReader/FilmInitializeState_Hook.h"
#include "Core/Hooks/Lifecycle/DestroySubsystems_Hook.h"
#include "Core/Hooks/MovReader/BlamOpenFile_Hook.h"
#include "External/minhook/include/MinHook.h"

DestroySubsystems_t original_DestroySubsystems = nullptr;
std::atomic<bool> g_DestroySubsystems_Hook_Installed = false;
void* g_DestroySubsystems_Address = nullptr;

void __fastcall hkDestroySubsystems(void)
{
	UIBuildDynamicMessage_Hook::Uninstall();
	UpdateTelemetryTimer_Hook::Uninstall();
	SpectatorHandleInput_Hook::Uninstall();
	FilmInitializeState_Hook::Uninstall();
	GetButtonState_Hook::Uninstall();
	BlamOpenFile_Hook::Uninstall();

	g_pState->Theater.SetTimePtr(nullptr);
	g_pState->Theater.SetTimeScalePtr(nullptr);

	g_pState->Lifecycle.SetEngineStatus({ EngineStatus::Destroyed });

	original_DestroySubsystems();
}

bool DestroySubsystems_Hook::Install(bool silent)
{
	if (g_DestroySubsystems_Hook_Installed.load()) return true;

	void* methodAddress = (void*)Scanner::FindPattern(Signatures::DestroySubsystems);
	if (!methodAddress)
	{
		if (!silent) Logger::LogAppend("Failed to obtain the address of DestroySubsystems()");
		return false;
	}

	g_DestroySubsystems_Address = methodAddress;

	MH_RemoveHook(g_DestroySubsystems_Address);

	MH_STATUS status = MH_CreateHook(g_DestroySubsystems_Address, &hkDestroySubsystems, reinterpret_cast<LPVOID*>(&original_DestroySubsystems));
	if (status != MH_OK)
	{
		Logger::LogAppend(std::string("Failed to create DestroySubsystems hook: " + std::to_string((int)status)).c_str());
		return false;
	}

	if (MH_EnableHook(g_DestroySubsystems_Address) != MH_OK) {
		Logger::LogAppend("Failed to enable DestroySubsystems hook");
		return false;
	}

	g_DestroySubsystems_Hook_Installed.store(true);
	Logger::LogAppend("DestroySubsystems hook installed");
	return true;
}

void DestroySubsystems_Hook::Uninstall()
{
	if (!g_DestroySubsystems_Hook_Installed.load()) return;

	MH_DisableHook(g_DestroySubsystems_Address);
	MH_RemoveHook(g_DestroySubsystems_Address);

	g_DestroySubsystems_Hook_Installed.store(false);
	Logger::LogAppend("DestroySubsystems hook uninstalled");
}