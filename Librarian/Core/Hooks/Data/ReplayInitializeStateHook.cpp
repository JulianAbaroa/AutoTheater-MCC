#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Infrastructure/Engine/FormatSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Hooks/Data/ReplayInitializeStateHook.h"
#include "External/minhook/include/MinHook.h"

// This function is triggered when the game engine initializes a Theater replay session.
// It provides access to the 'FLMH' (Replay Header) buffer, which contains the 
// initial snapshot of the session's state before the simulation starts.
// AutoTheater uses this hook to:
// 1. Extract the Base Map name (located at +0x930).
// 2. Build the Initial Player Registry: It parses the 16-slot player table starting 
//    at +0xBD0 (where each entry is 0xA0 bytes).
// 3. Populate Global State: Stores names and service ids into the Theater State.
// Note: This data reflects the players present at the moment the recording started.
void ReplayInitializeStateHook::HookedReplayInitializeState(uint64_t sessionContext, uint64_t headerBuffer)
{
	if (headerBuffer != 0)
	{
		g_pSystem->Debug->Log("[ReplayInitializeState] INFO: Analyzing FLMH from disk.");

		char* mapName = reinterpret_cast<char*>(headerBuffer + 0x930);
		 g_pSystem->Debug->Log("[ReplayInitializeState] INFO: Base map: %s.", mapName);
		 g_pSystem->Debug->Log("[ReplayInitializeState] INFO: Players present at joining:");

		std::vector<int> missingIndices;
		std::vector<PlayerInfo> firstPlayerList;
		firstPlayerList.resize(16);

		for (uint8_t i = 0; i < 16; i++)
		{
			uintptr_t playerOffset = headerBuffer + 0xBD0 + (i * 0xA0);
			wchar_t* wName = reinterpret_cast<wchar_t*>(playerOffset);
		
			if (wName[0] != 0 && iswprint(wName[0]))
			{
				std::string finalName = ToCompactAlpha(wName);
		
				wchar_t* wTag = reinterpret_cast<wchar_t*>(playerOffset + 0x44);
				std::string tag = g_pSystem->Infrastructure->Format->WStringToString(wTag);

				PlayerInfo info;
				info.Name = finalName;
				info.Tag = tag;
				info.Id = i;
		
				firstPlayerList[i] = info;
		
				g_pSystem->Debug->Log("[ReplayInitializeState] INFO: Player [%d]: %s [%s].", i, finalName.c_str(), tag.c_str());
			}
			else
			{
				missingIndices.push_back(i);
			}
		}

		g_pState->Domain->Theater->SetPlayerList(firstPlayerList);

		if (!missingIndices.empty())
		{
			char listBuffer[256]{};
			int offset = 0;

			for (size_t k = 0; k < missingIndices.size(); ++k)
			{
				int written = snprintf(listBuffer + offset, sizeof(listBuffer) - offset,
					"%d%s", (int)missingIndices[k], (k < missingIndices.size() - 1 ? ", " : ""));

				if (written < 0) break;
				offset += written;

				if (offset >= (int)sizeof(listBuffer)) break;
			}

			g_pSystem->Debug->Log("[ReplayInitializeState] INFO: Slots [%s] are empty in header.", listBuffer);
		}
	}

	m_OriginalFunction(sessionContext, headerBuffer);
}

std::string ReplayInitializeStateHook::ToCompactAlpha(const std::wstring& ws)
{
	std::string s;
	s.reserve(ws.length());
	for (wchar_t wc : ws)
	{
		if (wc > 0 && wc < 127 && iswprint(wc) && wc != L' ')
		{
			s += static_cast<char>(std::tolower(static_cast<unsigned char>(wc)));
		}
	}
	return s;
}


void ReplayInitializeStateHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = (void*)g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::ReplayInitializeState);
	if (!functionAddress)
	{
		g_pSystem->Debug->Log("[ReplayInitializeState] ERROR: Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedReplayInitializeState, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pSystem->Debug->Log("[ReplayInitializeState] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pSystem->Debug->Log("[ReplayInitializeState] ERROR: Failed to enable the hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[ReplayInitializeState] INFO: Hook installed.");
}

void ReplayInitializeStateHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[ReplayInitializeState] INFO: Hook uninstalled.");
}