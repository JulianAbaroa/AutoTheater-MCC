#include "pch.h"
#include "Core/Hooks/Scanner.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Hooks/Data/FilmInitializeStateHook.h"
#include "External/minhook/include/MinHook.h"

// This function is triggered when the game engine initializes a Theater film session.
// It provides access to the 'FLMH' (Film Header) buffer, which contains the 
// initial snapshot of the session's state before the simulation starts.
// AutoTheater uses this hook to:
// 1. Extract the Base Map name (located at +0x930).
// 2. Build the Initial Player Registry: It parses the 16-slot player table starting 
//    at +0xBD0 (where each entry is 0xA0 bytes).
// 3. Populate Global State: Stores names and service ids into the Theater State.
// Note: This data reflects the players present at the moment the recording started.
void FilmInitializeStateHook::HookedFilmInitializeState(uint64_t sessionContext, uint64_t headerBuffer)
{
	if (headerBuffer != 0)
	{
		g_pUtil->Log.Append("[FilmInitializeState] INFO: Analyzing FLMH from disk.");

		char* mapName = reinterpret_cast<char*>(headerBuffer + 0x930);
		g_pUtil->Log.Append("[FilmInitializeState] INFO: Base map: %s.", mapName);
		g_pUtil->Log.Append("[FilmInitializeState] INFO: Players present at joining:");

		std::vector<int> missingIndices;
		std::vector<PlayerInfo> firstPlayerList;
		firstPlayerList.resize(16);

		for (uint8_t i = 0; i < 16; i++)
		{
			uintptr_t playerOffset = headerBuffer + 0xBD0 + (i * 0xA0);
			wchar_t* wName = reinterpret_cast<wchar_t*>(playerOffset);
		
			if (wName[0] != 0 && iswprint(wName[0]))
			{
				std::string finalName = g_pUtil->Format.ToCompactAlpha(wName);
		
				wchar_t* wTag = reinterpret_cast<wchar_t*>(playerOffset + 0x44);
				std::string tag = g_pUtil->Format.WStringToString(wTag);

				PlayerInfo info;
				info.Name = finalName;
				info.Tag = tag;
				info.Id = i;
		
				firstPlayerList[i] = info;
		
				g_pUtil->Log.Append("[FilmInitializeState] INFO: Player [%d]: %s [%s].", i, finalName, tag);
			}
			else
			{
				missingIndices.push_back(i);
			}
		}

		g_pState->Theater.SetPlayerList(firstPlayerList);

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

			g_pUtil->Log.Append("[FilmInitializeState] WARNING: Slots [%s] are empty in header.", listBuffer);
		}
	}

	m_OriginalFunction(sessionContext, headerBuffer);
}

void FilmInitializeStateHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = (void*)Scanner::FindPattern(Signatures::FilmInitializeState);
	if (!functionAddress)
	{
		g_pUtil->Log.Append("[FilmInitializeState] ERROR: Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedFilmInitializeState, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pUtil->Log.Append("[FilmInitializeState] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pUtil->Log.Append("[FilmInitializeState] ERROR: Failed to enable the hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	g_pUtil->Log.Append("[FilmInitializeState] INFO: Hook installed.");
}

void FilmInitializeStateHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pUtil->Log.Append("[FilmInitializeState] INFO: Hook uninstalled.");
}