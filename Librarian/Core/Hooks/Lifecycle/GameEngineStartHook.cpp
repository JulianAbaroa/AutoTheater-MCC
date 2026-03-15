#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Hooks/Lifecycle/GameEngineStartHook.h"
#include "External/minhook/include/MinHook.h"

// This is the entry point for the Game Engine's primary initialization loop.
// It is triggered whenever a new simulation state is being prepared.
// Detection Logic: AutoTheater inspects 'param_3' for a specific 16-byte session 
// signature (CGB Theater Signature). This signature uniquely identifies if the 
// current engine state is a Theater replay session.
// Purpose: This prevents the mod from activating during normal gameplay (Campaign/MP), 
// ensuring that AutoTheater's director and hooks only engage when a valid 
// replay context is detected.
void __fastcall GameEngineStartHook::HookedGameEngineStart(
	uint64_t param_1, uint64_t param_2, uint64_t* param_3)
{
	m_OriginalFunction(param_1, param_2, param_3);

	if (memcmp(param_3, m_TheaterSignature, 16) == 0)
	{
		g_pSystem->Debug->Log("[GameEngineStart] INFO: Theater detected, proceding.");
		g_pState->Domain->Theater->SetTheaterMode(true);
	}
	else
	{
		g_pSystem->Debug->Log("[GameEngineStart] WARNING: Theater wasn't detected, aborting.");
		g_pState->Domain->Theater->SetTheaterMode(false);
	}
}

bool GameEngineStartHook::Install(bool silent)
{
	if (m_IsHookInstalled.load()) return true;

	void* functionAddress = (void*)g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::GameEngineStart);
	if (!functionAddress)
	{
		if (!silent)  g_pSystem->Debug->Log("[GameEngineStart] ERROR: Failed to obtain the function address.");
		return false;
	}

	m_FunctionAddress.store(functionAddress);
	MH_RemoveHook(m_FunctionAddress.load());
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedGameEngineStart, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pSystem->Debug->Log("[GameEngineStart] ERROR: Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		g_pSystem->Debug->Log("[GameEngineStart] ERROR: Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[GameEngineStart] INFO: Hook installed.");
	return true;
}

void GameEngineStartHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[GameEngineStart] INFO: Hook uninstalled.");
}

void* GameEngineStartHook::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}