#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Core/Scanner/Scanner.h"
#include "Hooks/Data/GetButtonState_Hook.h"
#include "Hooks/Lifecycle/DestroySubsystems_Hook.h"

#include "Hooks/MovReader/BlamOpenFile_Hook.h"
#include "Hooks/MovReader/FilmInitializeState_Hook.h"
#include "Hooks/Data/SpectatorHandleInput_Hook.h"
#include "Hooks/Data/UpdateTelemetryTimer_Hook.h"
#include "Hooks/Data/UIBuildDynamicMessage_Hook.h"

bool g_GameEngineDestroyed = false;

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

	g_GameEngineDestroyed = true;
	original_DestroySubsystems();
}

bool DestroySubsystems_Hook::Install()
{
	if (g_DestroySubsystems_Hook_Installed.load()) return true;

	void* methodAddress = (void*)Scanner::FindPattern(Signatures::DestroySubsystems);
	if (!methodAddress)
	{
		Logger::LogAppend("Failed to obtain the address of DestroySubsystems()");
		return false;
	}

	g_DestroySubsystems_Address = methodAddress;

	if (MH_CreateHook(g_DestroySubsystems_Address, &hkDestroySubsystems, reinterpret_cast<LPVOID*>(&original_DestroySubsystems)) != MH_OK)
	{
		Logger::LogAppend("Failed to create DestroySubsystems hook");
		return false;
	}

	if (MH_EnableHook(g_DestroySubsystems_Address) != MH_OK)
	{
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