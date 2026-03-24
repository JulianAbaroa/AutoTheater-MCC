#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Timeline/TimelineState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Domain/Director/EventRegistryState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Timeline/TimelineSystem.h"

void TimelineSystem::ProcessEngineEvent(float timestamp, std::wstring& templateStr, EventData* rawData)
{
	if (g_pSystem->Domain->Timeline->HasReachedLastEvent()) return;

	EventType type = g_pState->Domain->EventRegistry->GetEventType(templateStr);
	std::vector<PlayerInfo> players = ExtractPlayers(rawData);

	EventTeams teams = { rawData->CauseTeam, rawData->EffectTeam };

	GameEvent event { 
		timestamp, 
		type, 
		{ rawData->CauseTeam, rawData->EffectTeam }, 
		players 
	};

	if (this->IsDuplicate(event)) return;

	g_pState->Domain->Timeline->AddGameEvent(event);

	if (event.Type == EventType::Wins) g_pSystem->Domain->Timeline->SetLastEventReached(true);
}

std::vector<PlayerInfo> TimelineSystem::ExtractPlayers(EventData* rawData)
{
	std::vector<PlayerInfo> detectedPlayers;
	if (!rawData) return detectedPlayers;

	auto AddPlayerBySlot = [&](uint8_t slotIndex) {
		auto player = g_pState->Domain->Theater->GetPlayerBySlot(slotIndex);

		if (player && player->RawPlayer.Name[0] != L'\0')
		{
			detectedPlayers.push_back(*player);
			return true;
		}

		return false;
	};

	AddPlayerBySlot(rawData->CauseSlotIndex);

	if (rawData->EffectSlotIndex != rawData->CauseSlotIndex)
	{
		AddPlayerBySlot(rawData->EffectSlotIndex);
	}

	return detectedPlayers;
}

bool TimelineSystem::IsDuplicate(const GameEvent& newEvent)
{
	for (const auto& pastEvent : m_DeduplicationHistory)
	{
		bool sameTime = std::abs(pastEvent.Timestamp - newEvent.Timestamp) < 0.05f;
		if (!sameTime) continue;

		bool sameType = (pastEvent.Type == newEvent.Type);
		if (!sameType) continue;

		bool samePlayers = false;
		if (pastEvent.Players.size() == newEvent.Players.size()) 
		{
			if (pastEvent.Players.empty()) 
			{
				samePlayers = true;
			}
			else 
			{
				int matchCount = 0;

				for (const auto& pCurrent : newEvent.Players) 
				{
					for (const auto& pPast : pastEvent.Players) 
					{
						if (pCurrent.Name == pPast.Name) 
						{
							matchCount++;
							break;
						}
					}
				}

				if (matchCount == newEvent.Players.size()) samePlayers = true;
			}
		}

		if (samePlayers) return true;
	}

	m_DeduplicationHistory.push_back(newEvent);
	if (m_DeduplicationHistory.size() > m_MaxHistory) 
	{
		m_DeduplicationHistory.pop_front();
	}

	return false;
}


bool TimelineSystem::HasReachedLastEvent() const
{
	return m_LastEventReached.load();
}

void TimelineSystem::SetLastEventReached(bool value)
{
	m_LastEventReached.store(value);
}