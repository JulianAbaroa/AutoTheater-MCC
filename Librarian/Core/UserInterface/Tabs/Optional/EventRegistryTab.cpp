#include "pch.h"
#include "Utils/Formatting.h"
#include "Core/Common/AppCore.h"
#include "Core/Common/PersistenceManager.h"
#include "Core/UserInterface/Tabs/Optional/EventRegistryTab.h"
#include "External/imgui/imgui_internal.h"
#include "External/imgui/imgui.h"
#include <algorithm>
#include <string>
#include <set>

void EventRegistryTab::Draw()
{
	static char filter[64] = "";
	static std::vector<EventInfo> uniqueTypes;
	static bool needsRefresh = true;

	ImGui::AlignTextToFramePadding();
	ImGui::TextDisabled("Search:");
	ImGui::SameLine();

	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 100.0f);
	if (ImGui::InputTextWithHint("##filter", "Type to filter events (e.g. 'OverKill')...", filter, IM_ARRAYSIZE(filter)))
	{

	}
	ImGui::PopItemWidth();

	ImGui::SameLine();
	if (ImGui::Button("Clear", ImVec2(80.0f, 0.0f))) filter[0] = '\0';

	ImGui::Separator();
	ImGui::Spacing();

	if (needsRefresh && !ImGui::IsAnyItemActive())
	{
		uniqueTypes.clear();
		std::set<EventType> seen;

		g_pState->EventRegistry.ForEachEvent([&](const std::wstring& name, const EventInfo& info)
		{
			if (seen.find(info.Type) == seen.end())
			{
				uniqueTypes.push_back(info);
				seen.insert(info.Type);
			}
		});

		needsRefresh = false;
	}

	if (ImGui::BeginChild("RegistryScroll", ImVec2(0.0f, 0.0f), false))
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

			if (item.Weight > 50) ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), " %s", name.c_str());
			else				  ImGui::Text(" %s", name.c_str());

			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				const auto& eventDb = Formatting::GetEventDb();
				auto it = eventDb.find(item.Type);
				if (it != eventDb.end())
				{
					ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Game Event Details");
					ImGui::Separator();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
					ImGui::TextUnformatted(it->second.Description.c_str());
					ImGui::PopTextWrapPos();

					if (!it->second.InternalTemplates.empty()) {
						ImGui::Spacing();
						ImGui::TextDisabled("Engine Templates:");
						for (const auto& t : it->second.InternalTemplates) ImGui::BulletText("%ls", t.c_str());
					}
				}
				ImGui::EndTooltip();
			}

			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 220.0f);
			ImGui::SetNextItemWidth(210.0f);

			if (ImGui::SliderInt("##slider", &item.Weight, 0, 100, "Weight: %d")) 
			{
				g_pState->EventRegistry.UpdateWeightsByType(item.Type, item.Weight);
			}

			if (ImGui::IsItemDeactivatedAfterEdit()) {
				g_pSystem->EventRegistry.SaveEventRegistry();
				needsRefresh = true;
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
			static const EventClass categories[] = {
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

			for (EventClass cat : categories)
			{ 
				const char* catName = Formatting::GetEventClassName(cat);

				if (ImGui::CollapsingHeader(catName))
				{
					ImGui::Indent(10.0f);
					for (auto& item : uniqueTypes)
					{
						if (item.Class == cat)
						{
							DrawRow(item);
						}
					}
					ImGui::Unindent(10.0f);
				}
			}
		}

		ImGui::EndChild();
	}
}