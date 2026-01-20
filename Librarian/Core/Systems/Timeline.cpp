#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Systems/Timeline.h"
#include <algorithm>

std::vector<GameEvent> g_Timeline;
std::mutex g_TimelineMutex;

bool g_IsLastEvent = false;

static EventType GetEventType(const std::wstring& templateStr)
{
	for (const auto& er : g_EventRegistry)
	{
		if (templateStr.find(er.first) != std::wstring::npos)
		{
			return er.second.Type;
		}
	}

	return EventType::Unknown;
}

static std::vector<PlayerInfo> GetPlayers(EventData* eventData) {
	std::vector<PlayerInfo> detectedPlayers;
	if (!eventData) return detectedPlayers;

	const uint32_t INVALID_HANDLE = 0xFFFFFFFF;
	const uint32_t ZERO_HANDLE = 0;

	PlayerInfo* instigator = nullptr;
	PlayerInfo* victim = nullptr;

	for (auto& player : g_PlayerList)
	{
		uint32_t current = player.RawPlayer.BipedHandle;
		uint32_t previous = player.RawPlayer.PreviousBipedHandle;

		if (eventData->CauseHandle != INVALID_HANDLE && eventData->CauseHandle != ZERO_HANDLE)
		{
			if (current == eventData->CauseHandle || previous == eventData->CauseHandle)
			{
				instigator = &player;
			}
		}

		if (eventData->EffectHandle != INVALID_HANDLE && eventData->EffectHandle != ZERO_HANDLE)
		{
			if (current == eventData->EffectHandle || previous == eventData->EffectHandle)
			{
				victim = &player;
			}
		}

		if (instigator && victim) break;
	}

	if (instigator) detectedPlayers.push_back(*instigator);
	if (victim && victim != instigator) detectedPlayers.push_back(*victim);

	return detectedPlayers;
}

static bool IsRepeatedEvent(GameEvent gameEvent)
{
	static std::vector<GameEvent> eventHistory;
	const size_t MAX_HISTORY = 10;

	for (const auto& pastEvent : eventHistory)
	{
		bool sameTime = std::abs(pastEvent.Timestamp - gameEvent.Timestamp) < 0.05f;
		if (!sameTime) continue;

		bool sameType = (pastEvent.Type == gameEvent.Type);
		if (!sameType) continue;

		bool samePlayers = false;
		if (pastEvent.Players.size() == gameEvent.Players.size()) {
			if (pastEvent.Players.empty()) {
				samePlayers = true;
			}
			else {
				int matchCount = 0;
				for (const auto& pCurrent : gameEvent.Players) {
					for (const auto& pPast : pastEvent.Players) {
						if (pCurrent.Name == pPast.Name) {
							matchCount++;
							break;
						}
					}
				}
				if (matchCount == gameEvent.Players.size()) samePlayers = true;
			}
		}

		if (samePlayers) return true;
	}

	eventHistory.push_back(gameEvent);
	if (eventHistory.size() > MAX_HISTORY) {
		eventHistory.erase(eventHistory.begin());
	}

	return false;
}

void Timeline::AddGameEvent(float timestamp, std::wstring& templateStr, EventData* eventData)
{
	if (g_IsLastEvent) return;
	
	EventType currentType = GetEventType(templateStr);
	std::vector<PlayerInfo> currentPlayers = GetPlayers(eventData);
	Teams teams = { eventData->CauseTeam, eventData->EffectTeam };
	
	GameEvent gameEvent;
	gameEvent.Timestamp = timestamp;
	gameEvent.Type = currentType;
	gameEvent.Teams = teams;
	gameEvent.Players = currentPlayers;
	
	if (IsRepeatedEvent(gameEvent)) return;
	
	{
		std::lock_guard<std::mutex> lock(g_TimelineMutex);
		g_Timeline.push_back(gameEvent);
	}
	
	if (gameEvent.Type == EventType::Wins) g_IsLastEvent = true;
}