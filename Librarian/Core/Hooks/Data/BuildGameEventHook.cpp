#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Domain/Director/EventRegistryState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Timeline/TimelineSystem.h"
#include "Core/Systems/Domain/Theater/TheaterSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Infrastructure/Engine/FormatSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Hooks/Data/BuildGameEventHook.h"
#include "External/minhook/include/MinHook.h"

// This function acts as a string formatter for the in-game event feed (kill-feed).
// It processes a 'pTemplateStr' and uses 'pEventData' to generate the final localized
// string displayed to players.
// Observed Behavior: This hook does not capture every game event. While it processes
// major combat and world events, it does not appear to trigger for minor local 
// actions such as picking up weapons or equipment.
// Critical for AutoTheater: This hook is the primary data source for the Timeline.
// By intercepting these events, AutoTheater builds the real-time script that
// drives the automated camera director and replay editing.
unsigned char BuildGameEventHook::HookedBuildGameEvent(
	int playerMask, wchar_t* pTemplateStr, void* pEventData,
	unsigned int flags, wchar_t* pOutBuffer) 
{
	unsigned char result = m_OriginalFunction(playerMask, pTemplateStr, pEventData, flags, pOutBuffer);
	if (!result || !pOutBuffer || pOutBuffer[0] == L'\0') return result;
	if (g_pSystem->Domain->Timeline->HasReachedLastEvent() || !g_pState->Domain->Theater->GetTimePtr() || !pEventData) return result;

	EventData* eventData = (EventData*)pEventData;
	std::wstring currentTemplate(pTemplateStr);
	float currentTime = (g_pState->Domain->Theater->GetTimePtr() != nullptr) ? (float)*g_pState->Domain->Theater->GetTimePtr() : 0.0f;

	PrintNewEvent(pTemplateStr, pEventData, pOutBuffer, currentTemplate, currentTime);
	//PrintRawEvent(pOutBuffer);

	g_pSystem->Domain->Theater->RefreshPlayerList();
	g_pSystem->Domain->Timeline->ProcessEngineEvent(currentTime, currentTemplate, eventData);

	return result;
}

void BuildGameEventHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = (void*)g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::BuildGameEvent);
	if (!functionAddress)
	{
		g_pSystem->Debug->Log("[BuildGameEvent] ERROR: Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedBuildGameEvent, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pSystem->Debug->Log("[BuildGameEvent] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pSystem->Debug->Log("[BuildGameEvent] ERROR: Failed to enable hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[BuildGameEvent] INFO: Hook installed.");
}

void BuildGameEventHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[BuildGameEvent] INFO: Hook uninstalled.");
}


void BuildGameEventHook::PrintNewEvent(wchar_t* pTemplateStr, void* pEventData,
	wchar_t* pOutBuffer, std::wstring currentTemplate, float currentTime)
{
	bool alreadyMapped = g_pState->Domain->EventRegistry->IsEventRegistered(currentTemplate);
	if (!alreadyMapped) 
	{
		EventData* data = (EventData*)pEventData;
		std::string timeStr = g_pSystem->Infrastructure->Format->ToTimestamp(currentTime);

		g_pSystem->Debug->Log(
			"[BuildGameEvent] INFO: [%s] [NEW EVENT] Template: %ls | Msg: %ls | Slot: %d | Val: %d",
			timeStr.c_str(), pTemplateStr, pOutBuffer, data->CauseSlotIndex, data->CustomValue);
	}
}

void BuildGameEventHook::PrintRawEvent(wchar_t* pOutBuffer)
{
	g_pSystem->Debug->Log("[BuildGameEvent] INFO: RawEvent %ls", pOutBuffer);
}