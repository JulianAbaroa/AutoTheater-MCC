#include "pch.h"
#include "Utils/Formatting.h"
#include "Core/Common/GlobalState.h"
#include "Core/UserInterface/Tabs/EventRegistryTab.h"
#include "External/imgui/imgui_internal.h"
#include "External/imgui/imgui.h"
#include <algorithm>
#include <string>
#include <set>

// TODO: Maybe do a special struct instead of this
std::string GetEventCategory(EventType type) {
	if (type >= EventType::Join && type <= EventType::JoinedTeam) return "Server";
	if (type >= EventType::TookLead && type <= EventType::Wins) return "Match";
	if (type >= EventType::Custom && type <= EventType::Custom) return "Custom";
	if (type >= EventType::CaptureTheFlag && type <= EventType::FlagDropped) return "CTF";
	if (type >= EventType::Assault && type <= EventType::BombReset) return "Assault";
	if (type >= EventType::Slayer && type <= EventType::Slayer) return "Slayer";
	if (type >= EventType::Juggernaut && type <= EventType::YouKilledTheJuggernaut) return "Juggernaut";
	if (type >= EventType::Race && type <= EventType::Race) return "Race";
	if (type >= EventType::KingOfTheHill && type <= EventType::HillMoved) return "KOTH";
	if (type >= EventType::Territories && type <= EventType::TerritoryLost) return "Territories";
	if (type >= EventType::Infection && type <= EventType::YouAreAZombie) return "Infection";
	if (type >= EventType::Oddball && type <= EventType::BallDropped) return "Oddball";
	if (type >= EventType::Headshot && type <= EventType::Broseidon) return "Kills/Medals";
	return "Others";
}

void EventRegistryTab::Draw()
{
	static char filter[64] = "";
	
	if (ImGui::InputTextWithHint("##filter", "Search events...", filter, IM_ARRAYSIZE(filter), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		ImGui::SetWindowFocus(NULL);
	}

	if (ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered() && ImGui::IsItemActivated())
	{
		ImGui::ClearActiveID();
	}

	ImGui::SameLine();
	if (ImGui::Button("Clear")) filter[0] = '\0';
	ImGui::Separator();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 12));

	static std::vector<EventInfo> uniqueTypes;
	static bool needsRefresh = true;

	if (needsRefresh && !ImGui::IsAnyItemActive())
	{
		uniqueTypes.clear();
		std::set<EventType> seen;

		std::lock_guard lock(g_pState->configMutex);
		for (auto& [name, info] : g_pState->eventRegistry)
		{
			if (seen.find(info.Type) == seen.end()) {
				uniqueTypes.push_back(info);
				seen.insert(info.Type);
			}
		}

		needsRefresh = false;
	}

	if (ImGui::BeginChild("RegistryScroll", ImVec2(0, 0), false))
	{
		std::string filterStr = filter;
		std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

		auto DrawRow = [&](EventInfo& item) {
			std::string name = Formatting::EventTypeToString(item.Type);
			std::string nameLower = name;
			std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

			if (!filterStr.empty() && nameLower.find(filterStr) == std::string::npos) return;

			ImGui::PushID((int)item.Type);

			ImGui::BeginGroup();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(name.c_str());

			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();

				const auto& eventDb = Formatting::GetEventDb();
				auto it = eventDb.find(item.Type);

				if (it != eventDb.end())
				{
					ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Event Description");

					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(it->second.Description.c_str());
					ImGui::PopTextWrapPos();

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Internal Templates:");
					for (const auto& t : it->second.InternalTemplates)
					{
						ImGui::BulletText("%ls", t.c_str());
					}
				}

				ImGui::EndTooltip();
			}

			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 240.0f);
			ImGui::SetNextItemWidth(240.0f);

			if (ImGui::SliderInt("##slider", &item.Weight, 0, 100)) {
				std::lock_guard lock(g_pState->configMutex);
				for (auto& [regName, regInfo] : g_pState->eventRegistry) {
					if (regInfo.Type == item.Type) regInfo.Weight = item.Weight;
				}
			}

			if (ImGui::IsItemDeactivatedAfterEdit()) {
				needsRefresh = true;
				//g_pState->SaveToAppData();
			}

			ImGui::EndGroup();
			ImGui::Separator();

			ImGui::PopID();
			};

		if (!filterStr.empty())
		{
			for (auto& item : uniqueTypes) DrawRow(item);
		}
		else
		{
			const char* categories[] = {
				"Server", "Match", "Custom",
				"CTF", "Assault", "Slayer",
				"Juggernaut", "Race", "KOTH",
				"Territories", "Infection",
				"Oddball", "Kills/Medals",
				"Others"
			};

			for (const char* catName : categories)
			{
				if (ImGui::CollapsingHeader(catName))
				{
					ImGui::Indent(10.0f);
					for (auto& item : uniqueTypes)
					{
						if (GetEventCategory(item.Type) == catName) DrawRow(item);
					}

					ImGui::Unindent(10.0f);
					ImGui::Spacing();
				}
			}
		}

		ImGui::EndChild();
	}

	ImGui::PopStyleVar();
}