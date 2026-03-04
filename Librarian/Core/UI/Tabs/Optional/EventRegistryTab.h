#pragma once

#include "Core/Common/Types/TimelineTypes.h"
#include <vector>
#include <atomic>

class EventRegistryTab
{
public:
	void Draw();

private:
	void RefreshEventCache();
	void DrawEventRow(EventInfo& item, const std::string& filter);
	void DrawEventTooltip(EventInfo& item);
	void DrawCategoryView(const std::string& filter);
	void DrawFilteredView(const std::string& filter);

	void DrawSearchBar(char* buffer, size_t bufferSize);

	char m_SearchBuffer[128] = "";

	std::vector<EventInfo> m_UniqueTypes{};
	std::atomic<bool> m_NeedsRefresh{ true };

	const EventClass m_Categories[13] = {
		EventClass::Server,
		EventClass::Match,
		EventClass::Custom,
		EventClass::CaptureTheFlag,
		EventClass::Assault,
		EventClass::Slayer,
		EventClass::Juggernaut,
		EventClass::Race,
		EventClass::KingOfTheHill,
		EventClass::Territories,
		EventClass::Infection,
		EventClass::Oddball,
		EventClass::KillRelated
	};
};