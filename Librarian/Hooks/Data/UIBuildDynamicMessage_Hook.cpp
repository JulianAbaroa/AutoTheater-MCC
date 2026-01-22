#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Scanner/Scanner.h"
#include "Core/Systems/Timeline.h"
#include "Hooks/Data/SpectatorHandleInput_Hook.h"
#include "Hooks/Data/UpdateTelemetryTimer_Hook.h"
#include "Hooks/Data/UIBuildDynamicMessage_Hook.h"
#include "External/minhook/include/MinHook.h"

UIBuildDynamicMessage_t original_UIBuildDynamicMessage = nullptr;
std::atomic<bool> g_UIBuildDynamicMessage_Hook_Installed;
void* g_UIBuildDynamicMessage_Address;

std::wstring lastMsg = L"";
float lastReplaySeconds = -1.0f;
std::vector<std::wstring> tickMessageHistory;

static std::wstring lastTemplate;
static float lastEventTime = -1.0f;

static void PrintNewEvent(
	wchar_t* pTemplateStr,
	void* pEventData, 
	wchar_t* pOutBuffer,
	std::wstring currentTemplate, 
	float currentTime
) {
	bool alreadyMapped = false;
	for (const auto& entry : g_EventRegistry) {
		if (entry.first == currentTemplate) {
			alreadyMapped = true;
			break;
		}
	}

	if (!alreadyMapped) {
		lastEventTime = currentTime;
		lastTemplate = currentTemplate;

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

unsigned char hkUIBuildDynamicMessage(
	int playerMask,
	wchar_t* pTemplateStr,
	void* pEventData,
	unsigned int flags,
	wchar_t* pOutBuffer
) {
	unsigned char result = original_UIBuildDynamicMessage(playerMask, pTemplateStr, pEventData, flags, pOutBuffer);
	if (!result || !pOutBuffer || pOutBuffer[0] == L'\0') return result;
	if (g_IsLastEvent || !g_pReplayTime || !pEventData) return result;

	EventData* eventData = (EventData*)pEventData;
	std::wstring currentTemplate(pTemplateStr);
	float currentTime = (g_pReplayTime != nullptr) ? (float)*g_pReplayTime : 0.0f;

	PrintNewEvent(pTemplateStr, pEventData, pOutBuffer, currentTemplate, currentTime);
	
	//char finalMsg[512];
	//snprintf(finalMsg, sizeof(finalMsg), "%ls", pOutBuffer);
	//Logger::LogAppend(finalMsg);

	Theater::RebuildPlayerListFromMemory();
	Timeline::AddGameEvent(currentTime, currentTemplate, eventData);

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