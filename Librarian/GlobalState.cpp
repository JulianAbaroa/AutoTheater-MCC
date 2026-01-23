#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/GlobalState.h"
#include <fstream>

AppState g_State;

void AppState::SaveTimeline(std::string replayName)
{
	std::string path;

	{
		std::lock_guard lock(g_State.configMutex);
		path = g_State.baseDirectory + "\\Replays\\" + replayName + ".timeline";
	}

	std::ofstream file(path, std::ios::binary);

	if (!file.is_open()) return;

	auto saveString = [&](const std::string& s) {
		size_t len = s.length();
		file.write((char*)&len, sizeof(len));
		file.write(s.c_str(), len);
	};

	std::vector<GameEvent> timelineCopy;

	{
		std::lock_guard lock(timelineMutex);
		timelineCopy = g_State.timeline;
	}

	size_t eventCount = timelineCopy.size();
	file.write((char*)&eventCount, sizeof(eventCount));
	
	for (const auto& ev : timelineCopy)
	{
		file.write((char*)&ev.Timestamp, sizeof(ev.Timestamp));
		file.write((char*)&ev.Type, sizeof(ev.Type));
		file.write((char*)&ev.Teams, sizeof(ev.Teams));
	
		size_t playerCount = ev.Players.size();
		file.write((char*)&playerCount, sizeof(playerCount));
	
		for (const auto& p : ev.Players)
		{
			file.write((char*)&p.Id, sizeof(p.Id));
	
			saveString(p.Name);
			saveString(p.Tag);
	
			file.write((char*)&p.RawPlayer, sizeof(RawPlayer));

			size_t weaponCount = p.Weapons.size();
			file.write((char*)&weaponCount, sizeof(weaponCount));
	
			for (const auto& w : p.Weapons)
			{
				file.write((char*)&w, sizeof(RawWeapon));
			}
		}
	}

	file.close();
	Logger::LogAppend("Timeline saved to AppData");
}

void AppState::LoadTimeline(std::string replayName)
{
	std::string path;

	{
		std::lock_guard lock(g_State.configMutex);
		path = g_State.baseDirectory + "\\Replays\\" + replayName + ".timeline";
	}

	std::ifstream file(path, std::ios::binary);

	if (!file.is_open()) {
		Logger::LogAppend("No cached timeline found for this replay.");
		return;
	}

	auto loadString = [&](std::string& s) {
		size_t len = 0;
		file.read((char*)&len, sizeof(len));
		if (len > 0 && len < 1024)
		{
			s.resize(len);
			file.read(&s[0], len);
		}
	};

	size_t eventCount = 0;
	std::vector<GameEvent> tempTimeline;

	{
		std::lock_guard lock(g_State.timelineMutex);
		g_State.timeline.clear();

		file.read((char*)&eventCount, sizeof(eventCount));
		g_State.timeline.reserve(eventCount);
	}
	
	for (size_t i = 0; i < eventCount; i++)
	{
		GameEvent ev;
	
		file.read((char*)&ev.Timestamp, sizeof(ev.Timestamp));
		file.read((char*)&ev.Type, sizeof(ev.Type));
		file.read((char*)&ev.Teams, sizeof(ev.Teams));

		size_t playerCount = 0;
		file.read((char*)&playerCount, sizeof(playerCount));
		ev.Players.reserve(playerCount);
	
		for (size_t j = 0; j < playerCount; j++)
		{
			PlayerInfo p;
			file.read((char*)&p.Id, sizeof(p.Id));
	
			loadString(p.Name);
			loadString(p.Tag);
	
			file.read((char*)&p.RawPlayer, sizeof(RawPlayer));

			size_t weaponCount = 0;
			file.read((char*)&weaponCount, sizeof(weaponCount));
			p.Weapons.resize(weaponCount);
		
			for (size_t k = 0; k < weaponCount; k++)
			{
				file.read((char*)&p.Weapons, sizeof(RawWeapon));
			}
	
			ev.Players.push_back(p);
		}
	
		tempTimeline.push_back(ev);
	}

	{
		std::lock_guard lock(g_State.timelineMutex);
		g_State.timeline = tempTimeline;
	}

	file.close();
	Logger::LogAppend("Timeline loaded successfully from AppData");

	// TODO: Maybe change right here to the Phase::ExecuteDirector
}

void AppState::SaveToAppData()
{

}

void AppState::LoadFromAppData()
{

}