#include "pch.h"
#include "Utils/Formatting.h"
#include "External/imgui/imgui.h"
#include "Core/Common/AppCore.h"
#include "Core/UserInterface/Tabs/Primary/TimelineTab.h"
#include <vector>
#include <string>

static void DrawTimelineControls(bool& autoScroll)
{
	ImGui::AlignTextToFramePadding();
	ImGui::Checkbox("Auto-Scroll", &autoScroll);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Keep the list scrolled to the most recent event.");	
	}
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
		"Enable/Disable writing events to the log file and Logs Tab."
	);
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

static void RenderPlayerCell(const std::vector<PlayerInfo>& players)
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
			(i < players.size() - 1) ? "," : ""
		);

		if (i < players.size() - 1) ImGui::SameLine();
	}
}

void TimelineTab::Draw()
{
	static bool autoScroll = true;
	static ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

	DrawTimelineControls(autoScroll);
	ImGui::Separator();

	if (ImGui::BeginChild("TimelineContent", ImVec2(0, 0), false))
	{
		if (ImGui::BeginTable("TimelineTable", 3, tableFlags))
		{
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Event Type", ImGuiTableColumnFlags_WidthFixed, 200.0f);
			ImGui::TableSetupColumn("Involved Players", ImGuiTableColumnFlags_WidthStretch);
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
					ImGui::TextUnformatted(Formatting::ToTimestamp(gameEvent.Timestamp).c_str());
			
					// Column: Event
					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(Formatting::EventTypeToString(gameEvent.Type).c_str());
			
					// Column: Players
					ImGui::TableSetColumnIndex(2);
					RenderPlayerCell(gameEvent.Players);
				}
			}

			if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			{
				ImGui::SetScrollHereY(1.0f);
			}

			ImGui::EndTable();
		}
	}

	ImGui::EndChild();
}