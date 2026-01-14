#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Systems/Timeline.h"
#include <algorithm>

std::vector<GameEvent> g_Timeline;
bool g_IsLastEvent = false;

static EventType GetEventType(const std::wstring& message)
{
	EventType bestType = EventType::Unknown;
	int highestWeight = -1;
	size_t longestMatch = 0;

	for (const auto& er : g_EventRegistry)
	{
		std::wstring keyword = er.second.Keyword;

		if (message.find(keyword) != std::wstring::npos)
		{
			int weight = er.second.Weight;
			size_t currentLength = keyword.length();

			if (weight > highestWeight || (weight == highestWeight && currentLength > longestMatch))
			{
				bestType = er.first;
				highestWeight = weight;
				longestMatch = currentLength;
			}
		}
	}

	return bestType;
}

static std::vector<PlayerInfo> GetPlayers(
	const std::wstring& cleanMessage, 
	unsigned int playerHandle
) {
	std::vector<PlayerInfo> detectedPlayers;
	if (cleanMessage.empty()) return detectedPlayers;

	// For the player that has the flag (or objective ?)
	if (playerHandle != 0 && playerHandle != 0xFFFFFFFF)
	{
		for (auto& player : g_PlayerList)
		{
			if (player.RawPlayer.BipedHandle == playerHandle)
			{
				player.IsVictim = false;
				detectedPlayers.push_back(player);
				return detectedPlayers;
			}
		}
	}

	struct Match { PlayerInfo info; size_t pos; };
	std::vector<Match> matches;

	for (const auto& player : g_PlayerList)
	{
		if (player.Name.empty()) continue;

		std::wstring cleanName = Formatting::ToCompactAlphaW(player.Name);

		size_t foundPos = cleanMessage.find(cleanName);
		if (foundPos != std::wstring::npos)
		{
			matches.push_back({ player, foundPos });
		}
	}
	
	if (matches.size() > 1) {
		std::sort(matches.begin(), matches.end(), [](const Match& a, const Match& b) {
			return a.pos < b.pos;
		});
	}
	
	for (size_t i = 0; i < matches.size(); ++i)
	{
		matches[i].info.IsVictim = (i > 0);
		detectedPlayers.push_back(matches[i].info);
	}

	return detectedPlayers;
}

static bool IsRepeatedEvent(GameEvent gameEvent) 
{
	static GameEvent lastEvent;
	static bool hasLastEvent = false;

	if (hasLastEvent)
	{
		if (std::abs(lastEvent.Timestamp - gameEvent.Timestamp) < 1.0f && lastEvent.Type == gameEvent.Type)
		{
			if (lastEvent.Players.size() == gameEvent.Players.size())
			{
				if (gameEvent.Players.empty()) return true;

				if (lastEvent.Players[0].Name == gameEvent.Players[0].Name) {
					return true;
				}
			}
		}
	}

	lastEvent = gameEvent;
	hasLastEvent = true;

	return false;
}

void Timeline::AddGameEvent(float timestamp, std::wstring eventText, unsigned int playerHandle=0)
{
	if (g_IsLastEvent) return;

	std::wstring cleanText = Formatting::ToCompactAlphaW(eventText);
	
	EventType currentType = GetEventType(cleanText);
	std::vector<PlayerInfo> currentPlayers = GetPlayers(cleanText, playerHandle);

	GameEvent gameEvent;
	gameEvent.Timestamp = timestamp;
	gameEvent.Type = currentType;
	gameEvent.Players = currentPlayers;

	if (IsRepeatedEvent(gameEvent)) return;

	g_Timeline.push_back(gameEvent);

	if (gameEvent.Type == EventType::Wins) g_IsLastEvent = true;
}