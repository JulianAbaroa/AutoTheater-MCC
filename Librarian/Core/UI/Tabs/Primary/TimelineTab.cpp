#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/UI/Tabs/Primary/TimelineTab.h"
#include "External/imgui/imgui.h"

void TimelineTab::Draw()
{
	bool autoScroll = m_AutoScroll.load();
	this->DrawTimelineControls(autoScroll);

	ImGui::Separator();

	if (ImGui::BeginChild("TimelineContent", ImVec2(0, 0), false))
	{
		ImVec4 tabActiveColor = ImGui::GetStyleColorVec4(ImGuiCol_TabActive);
		ImVec4 rowBgAlt = ImVec4(tabActiveColor.x, tabActiveColor.y, tabActiveColor.z, 0.05f);

		ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, tabActiveColor);
		ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, rowBgAlt);

		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, tabActiveColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, tabActiveColor);

		if (ImGui::BeginTable("TimelineTable", 3, m_TableFlags))
		{
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 100.0f);
			ImGui::TableSetupColumn("Event Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 200.0f);
			ImGui::TableSetupColumn("Involved Players", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort);
			ImGui::TableHeadersRow();

			const auto& timelineCopy = g_pState->Timeline.GetTimelineCopy();
			
			ImGuiListClipper clipper;
			clipper.Begin(static_cast<int>(timelineCopy.size()));
			
			while (clipper.Step())
			{
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
				{
					const auto& gameEvent = timelineCopy[i];
					ImGui::TableNextRow();
			
					// Column: Timestamp
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(g_pUtil->Format.ToTimestamp(gameEvent.Timestamp).c_str());
			
					// Column: Event
					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(g_pUtil->Format.EventTypeToString(gameEvent.Type).c_str());
			
					// Column: Players
					ImGui::TableSetColumnIndex(2);
					RenderPlayerCell(gameEvent.Players);
				}
			}

			if (m_AutoScroll.load() && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			{
				ImGui::SetScrollHereY(1.0f);
			}

			ImGui::EndTable();
		}

		ImGui::PopStyleColor(4);
	}

	ImGui::EndChild();
}

void TimelineTab::DrawTimelineControls(bool& autoScroll)
{
	ImGui::AlignTextToFramePadding();

	if (ImGui::Checkbox("Auto-Scroll", &autoScroll))
	{
		m_AutoScroll.store(autoScroll);
	}

	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Keep the list scrolled to the most recent event.");

	ImGui::SameLine();

	auto AtomicCheckBox = [](const char* label, auto getter, auto setter, const char* tooltip) {
		bool val = getter();
		if (ImGui::Checkbox(label, &val)) setter(val);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip(tooltip);
	};

	AtomicCheckBox(
		"Log Events",
		[&]() { return g_pState->Timeline.IsLoggingActive(); },
		[&](bool val) { g_pState->Timeline.SetLoggingActive(val); },
		"Enable/Disable writing events to the log file and Logs Tab.");

	ImGui::SameLine();

	AtomicCheckBox(
		"Last Event",
		[&]() { return g_pSystem->Timeline.HasReachedLastEvent(); },
		[&](bool val) { g_pSystem->Timeline.SetLastEventReached(val); },
		"Stop capturing any new GameEvents from the engine.");

	ImGui::SameLine();

	ImGui::TextDisabled("| Events: %zu", g_pState->Timeline.GetTimelineSize());
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Total number of events currently stored in memory.");
	}

	ImGui::SameLine();
	ImGui::TextDisabled("| Processed: %zu", g_pSystem->Timeline.GetLoggedEventsCount());

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Number of events successfully written to the disk/logs.");
	}
}

void TimelineTab::RenderPlayerCell(const std::vector<PlayerInfo>& players)
{
	if (players.empty())
	{
		ImGui::Text("-");
		return;
	}

	for (size_t i = 0; i < players.size(); ++i)
	{
		ImGui::Text(
			"%s [%s]%s",
			players[i].Name.c_str(),
			players[i].Tag.c_str(),
			(i < players.size() - 1) ? "," : "");

		if (i < players.size() - 1) ImGui::SameLine();
	}
}