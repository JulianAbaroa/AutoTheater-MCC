#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Common/AppCore.h"
#include "Core/Common/EventRegistry.h"
#include "Core/Common/Types/TimelineTypes.h"
#include "Core/Systems/Domain/TimelineSystem.h"
#include <algorithm>

void TimelineSystem::ProcessEngineEvent(float timestamp, std::wstring& templateStr, EventData* rawData)
{
	if (g_pSystem->Timeline.HasReachedLastEvent()) return;

	EventType type = g_pState->EventRegistry.GetEventType(templateStr);
	std::vector<PlayerInfo> players = ExtractPlayers(rawData);

	EventTeams teams = { rawData->CauseTeam, rawData->EffectTeam };

	GameEvent event { 
		timestamp, 
		type, 
		{ rawData->CauseTeam, rawData->EffectTeam }, 
		players 
	};

	if (IsDuplicate(event)) return;

	g_pState->Timeline.AddGameEvent(event);

	if (event.Type == EventType::Wins) g_pSystem->Timeline.SetLastEventReached(true);
}

std::vector<PlayerInfo> TimelineSystem::ExtractPlayers(EventData* rawData) {
	std::vector<PlayerInfo> detectedPlayers;
	if (!rawData) return detectedPlayers;

	std::vector<PlayerInfo> playerListCopy = g_pState->Theater.GetPlayerListCopy();

	size_t maxPlayers = playerListCopy.size();

	auto AddPlayerBySlot = [&](uint8_t slotIndex) {
		if (slotIndex < maxPlayers) {
			PlayerInfo& player = playerListCopy[slotIndex];

			if (player.RawPlayer.Name[0] != L'\0') {
				detectedPlayers.push_back(player);
				return true;
			}
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
		if (pastEvent.Players.size() == newEvent.Players.size()) {
			if (pastEvent.Players.empty()) {
				samePlayers = true;
			}
			else {
				int matchCount = 0;
				for (const auto& pCurrent : newEvent.Players) {
					for (const auto& pPast : pastEvent.Players) {
						if (pCurrent.Name == pPast.Name) {
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
	if (m_DeduplicationHistory.size() > MAX_HISTORY) {
		m_DeduplicationHistory.pop_front();
	}

	return false;
}

std::vector<GameEvent> TimelineSystem::ConsumePendingEvents()
{
	std::vector<GameEvent> allEvents = g_pState->Timeline.GetTimelineCopy();
	size_t lastLogged = g_pSystem->Timeline.GetLoggedEventsCount();
	size_t totalEvents = allEvents.size();

	if (lastLogged >= totalEvents) return {};

	std::vector<GameEvent> peding(allEvents.begin() + lastLogged, allEvents.end());

	g_pSystem->Timeline.SetLoggedEventsCount(totalEvents);

	return peding;
}

float TimelineSystem::GetLatestTimestamp() const
{
	std::vector<GameEvent> allEvents = g_pState->Timeline.GetTimelineCopy();

	if (allEvents.empty()) return 0.0f;
	return allEvents.back().Timestamp;
}


bool TimelineSystem::HasReachedLastEvent() const
{
	return m_LastEventReached.load();
}

size_t TimelineSystem::GetLoggedEventsCount() const
{
	return m_LoggedEventsCount.load();
}

void TimelineSystem::SetLastEventReached(bool value)
{
	m_LastEventReached.store(value);
}

void TimelineSystem::SetLoggedEventsCount(size_t value)
{
	m_LoggedEventsCount.store(value);
}