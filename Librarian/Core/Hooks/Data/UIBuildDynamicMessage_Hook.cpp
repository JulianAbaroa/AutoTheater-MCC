#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Common/AppCore.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include "Core/Systems/Domain/TheaterSystem.h"
#include "Core/Systems/Domain/TimelineSystem.h"
#include "Core/Hooks/Data/UIBuildDynamicMessage_Hook.h"
#include "External/minhook/include/MinHook.h"

UIBuildDynamicMessage_t original_UIBuildDynamicMessage = nullptr;
std::atomic<bool> g_UIBuildDynamicMessage_Hook_Installed;
void* g_UIBuildDynamicMessage_Address;

static void PrintNewEvent(
	wchar_t* pTemplateStr,
	void* pEventData, 
	wchar_t* pOutBuffer,
	std::wstring currentTemplate, 
	float currentTime
) {
	bool alreadyMapped = g_pState->EventRegistry.IsEventRegistered(currentTemplate);

	if (!alreadyMapped) {
		EventData* data = (EventData*)pEventData;
		char diagLog[512];
		std::string timeStr = Formatting::ToTimestamp(currentTime);

		snprintf(diagLog, sizeof(diagLog),
			"[%s] [NEW EVENT] Template: %ls | Msg: %ls | Slot: %d | Val: %d",
			timeStr.c_str(),
			pTemplateStr,
			pOutBuffer,
			data->CauseSlotIndex,
			data->CustomValue);

		Logger::LogAppend(diagLog);
	}
}

// This function acts as a string formatter for the in-game event feed (kill-feed).
// It processes a 'pTemplateStr' and uses 'pEventData' to generate the final localized
// string displayed to players.
// Observed Behavior: This hook does not capture every game event. While it processes
// major combat and world events, it does not appear to trigger for minor local 
// actions such as picking up weapons or equipment.
// Critical for AutoTheater: This hook is the primary data source for the Timeline.
// By intercepting these events, AutoTheater builds the real-time script that
// drives the automated camera director and replay editing.
unsigned char hkUIBuildDynamicMessage(
	int playerMask,
	wchar_t* pTemplateStr,
	void* pEventData,
	unsigned int flags,
	wchar_t* pOutBuffer
) {
	unsigned char result = original_UIBuildDynamicMessage(playerMask, pTemplateStr, pEventData, flags, pOutBuffer);
	if (!result || !pOutBuffer || pOutBuffer[0] == L'\0') return result;
	if (g_pSystem->Timeline.HasReachedLastEvent() || !g_pState->Theater.GetTimePtr() || !pEventData) return result;

	EventData* eventData = (EventData*)pEventData;
	std::wstring currentTemplate(pTemplateStr);
	float currentTime = (g_pState->Theater.GetTimePtr() != nullptr) ? (float)*g_pState->Theater.GetTimePtr() : 0.0f;

	PrintNewEvent(pTemplateStr, pEventData, pOutBuffer, currentTemplate, currentTime);
	
	//char finalMsg[512];
	//snprintf(finalMsg, sizeof(finalMsg), "%ls", pOutBuffer);
	//Logger::LogAppend(finalMsg);

	g_pSystem->Theater.RefreshPlayerList();
	g_pSystem->Timeline.ProcessEngineEvent(currentTime, currentTemplate, eventData);

	return result;
}

void UIBuildDynamicMessage_Hook::Install()
{
	if (g_UIBuildDynamicMessage_Hook_Installed.load()) return;

	void* methodAddress = (void*)Scanner::FindPattern(Signatures::UIBuildDynamicMessage);
	if (!methodAddress)
	{
		Logger::LogAppend("Failed to obtain the address of UIBuildDynamicMessage()");
		return;
	}

	g_UIBuildDynamicMessage_Address = methodAddress;
	
	if (MH_CreateHook(g_UIBuildDynamicMessage_Address, &hkUIBuildDynamicMessage, reinterpret_cast<LPVOID*>(&original_UIBuildDynamicMessage)) != MH_OK)
	{
		Logger::LogAppend("Failed to create UIBuildDynamicMessage hook");
		return;
	}

	if (MH_EnableHook(g_UIBuildDynamicMessage_Address) != MH_OK)
	{
		Logger::LogAppend("Failed to enalbe UIBuildDynamicMessage hook");
		return;
	}

	g_UIBuildDynamicMessage_Hook_Installed.store(true);
	Logger::LogAppend("UIBuildDynamicMessage hook installed");
}

void UIBuildDynamicMessage_Hook::Uninstall()
{
	if (!g_UIBuildDynamicMessage_Hook_Installed.load()) return;

	MH_DisableHook(g_UIBuildDynamicMessage_Address);
	MH_RemoveHook(g_UIBuildDynamicMessage_Address);

	g_UIBuildDynamicMessage_Hook_Installed.store(false);
	Logger::LogAppend("UIBuildDynamicMessage hook uninstalled");
}