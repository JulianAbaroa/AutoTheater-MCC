#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/UI/Tabs/Optional/EventRegistryTab.h"
#include <set>

void EventRegistryTab::Draw()
{
	this->DrawSearchBar(m_SearchBuffer, IM_ARRAYSIZE(m_SearchBuffer));
	this->RefreshEventCache();

	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::BeginChild("RegistryScroll", ImVec2(0.0f, 0.0f), false))
	{
		std::string filterStr = m_SearchBuffer;
		std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

		if (!filterStr.empty()) this->DrawFilteredView(filterStr);
		else this->DrawCategoryView(filterStr);

		ImGui::EndChild();
	}
}

void EventRegistryTab::RefreshEventCache()
{
	if ((!m_NeedsRefresh.load() && !m_UniqueTypes.empty()) || ImGui::IsAnyItemActive()) return;

	m_UniqueTypes.clear();
	std::set<EventType> seen;

	g_pState->EventRegistry.ForEachEvent([&](const std::wstring& name, const EventInfo& info) {
		if (seen.find(info.Type) == seen.end())
		{
			m_UniqueTypes.push_back(info);
			seen.insert(info.Type);
		}
	});

	m_NeedsRefresh.store(false);
}

void EventRegistryTab::DrawEventRow(EventInfo& item, const std::string& filter)
{
	std::string name = g_pUtil->Format.EventTypeToString(item.Type);

	if (!filter.empty())
	{
		std::string nameLower = name;
		std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

		if (nameLower.find(filter) == std::string::npos) return;
	}

	ImGui::PushID((int)item.Type + (int)item.Class * 1000);
	ImGui::BeginGroup();

	if (item.Weight > 50) ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), " %s", name.c_str());
	else ImGui::Text(" %s", name.c_str());

	if (ImGui::IsItemHovered()) this->DrawEventTooltip(item);

	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 220.0f);
	ImGui::SetNextItemWidth(210.0f);

	if (ImGui::SliderInt("##slider", &item.Weight, 0, 100, "Weight: %d"))
	{
		g_pState->EventRegistry.UpdateWeightsByType(item.Type, item.Weight);
	}

	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		g_pSystem->EventRegistry.SaveEventRegistry();
		m_NeedsRefresh.store(true);
	}

	ImGui::EndGroup();
	ImGui::Separator();
	ImGui::PopID();
}

void EventRegistryTab::DrawEventTooltip(EventInfo& item)
{
	ImGui::BeginTooltip();

	const auto& eventDb = g_pUtil->Format.GetEventDb();
	auto it = eventDb.find(item.Type);

	if (it != eventDb.end())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Game Event Details");
		ImGui::Separator();

		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
		ImGui::TextUnformatted(it->second.Description.c_str());
		ImGui::PopTextWrapPos();

		if (!it->second.InternalTemplates.empty())
		{
			ImGui::Spacing();
			ImGui::TextDisabled("Engine Templates:");
			for (const auto& t : it->second.InternalTemplates) ImGui::BulletText("%ls", t.c_str());
		}
	}

	ImGui::EndTooltip();
}

void EventRegistryTab::DrawFilteredView(const std::string& filter)
{
	for (auto& item : m_UniqueTypes)
	{
		this->DrawEventRow(item, filter);
	}
}

void EventRegistryTab::DrawCategoryView(const std::string& filter)
{
	for (EventClass category : m_Categories)
	{
		const char* categoryName = g_pUtil->Format.GetEventClassName(category);

		ImGui::PushID(categoryName);

		if (ImGui::CollapsingHeader(categoryName))
		{
			ImGui::Indent(10.0f);

			bool hasItems = false;
			for (auto& item : m_UniqueTypes)
			{
				if (item.Class == category)
				{
					this->DrawEventRow(item, filter);
					hasItems = true;
				}
			}

			if (!hasItems) ImGui::TextDisabled("No events in this category");

			ImGui::Unindent(10.0f);
		}
		ImGui::PopID();
	}
}


void EventRegistryTab::DrawSearchBar(char* buffer, size_t bufferSize)
{
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Filter:");
	ImGui::SameLine();

	float clearBtnWidth = 60.0f;
	float availableWidth = ImGui::GetContentRegionAvail().x;

	ImGui::PushItemWidth(availableWidth - clearBtnWidth - ImGui::GetStyle().ItemSpacing.x);
	if (ImGui::InputTextWithHint("##event_filter", "Search for specific events...", buffer, bufferSize)) {}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	if (ImGui::Button("Clear", ImVec2(clearBtnWidth, 0)))
	{
		buffer[0] = '\0';
	}
}