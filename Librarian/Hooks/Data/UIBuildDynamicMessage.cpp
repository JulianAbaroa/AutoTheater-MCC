#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Scanner/Scanner.h"
#include "Core/Systems/Theater.h"
#include "Core/Systems/Timeline.h"
#include "Hooks/Data/UIBuildDynamicMessage_Hook.h"
#include <iomanip>
#include <vector>

UIBuildDynamicMessage_t original_UIBuildDynamicMessage = nullptr;
std::atomic<bool> g_UIBuildDynamicMessage_Hook_Installed;
void* g_UIBuildDynamicMessage_Address;

std::wstring lastMsg = L"";
float lastReplaySeconds = -1.0f;
std::vector<std::wstring> tickMessageHistory;

unsigned char hkUIBuildDynamicMessage(
	int playerMask,
	wchar_t* pTemplateStr,
	void* pEventData,
	unsigned int flags,
	wchar_t* pOutBuffer
) {
	unsigned char result = original_UIBuildDynamicMessage(playerMask, pTemplateStr, pEventData, flags, pOutBuffer);
	if (g_IsLastEvent || !g_pReplayTime) return result;

	if (result && pOutBuffer && pOutBuffer[0] != L'\0')
	{
		std::wstring message(pOutBuffer);
		std::wstring lowerMsg = message;
		for (auto& c : lowerMsg) c = towlower(c);

		float currentTime = (g_pReplayTime) ? *g_pReplayTime : 0.0f;

		bool hasFlag = (lowerMsg.find(L"flag") != std::wstring::npos);
		bool hasYou = (lowerMsg.find(L"you") != std::wstring::npos);
		bool hasYour = (lowerMsg.find(L"your") != std::wstring::npos);
		bool hasWas = (lowerMsg.find(L"was") != std::wstring::npos);
		bool hasTeam = (lowerMsg.find(L"team") != std::wstring::npos);
		bool hasGrabbed = (lowerMsg.find(L"grabbed") != std::wstring::npos);

		unsigned int playerHandle = 0xFFFFFFFF;

		if (hasFlag)
		{
			if (!hasGrabbed && (hasYour || hasWas || hasTeam)) return result;

			if (pEventData != nullptr)
			{
				playerHandle = *(unsigned int*)((char*)pEventData + 0x4);
			}
		}
		else
		{
			if (hasYou || hasYour || hasWas) return result;
		}

		if (std::abs(currentTime - lastReplaySeconds) > 0.001f)
		{
			tickMessageHistory.clear();
			lastReplaySeconds = currentTime;
		}

		if (std::find(tickMessageHistory.begin(), tickMessageHistory.end(), message) == tickMessageHistory.end())
		{
			tickMessageHistory.push_back(message);

			Theater::RebuildPlayerListFromMemory();

			float totalTime = (g_pReplayModule != 0) ? (float)*g_pReplayTime : 0.0f;

			Timeline::AddGameEvent(totalTime, message, playerHandle);

			// To find new GameEvents
			char finalMsg[512];
			std::string timeStr = Formatting::ToTimestamp(totalTime);
			snprintf(finalMsg, sizeof(finalMsg), "[%s] %ls", timeStr.c_str(), pOutBuffer);
			Logger::LogAppend(finalMsg);
		}
	}

	return result;
}

void UIBuildDynamicMessage_Hook::Install()
{
	if (g_UIBuildDynamicMessage_Hook_Installed.load()) return;

	void* methodAddress = (void*)Scanner::FindPattern(Sig_UIBuildDynamicMessage);
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