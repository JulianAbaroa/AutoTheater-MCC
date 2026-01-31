#include "pch.h"
#include "Utils/Formatting.h"
#include "External/imgui/imgui.h"
#include "Core/Common/GlobalState.h"
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

	auto AtomicCheckBox = [](const char* label, std::atomic<bool>& value, const char* tooltip) {
		bool val = value.load();
		if (ImGui::Checkbox(label, &val)) value.store(val);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip(tooltip);
	};

	AtomicCheckBox("Log Events", g_pState->LogGameEvents,
		"Enable/Disable writing events to the log file and Logs Tab.");
	ImGui::SameLine();

	AtomicCheckBox("Last Event", g_pState->IsLastEvent,
		"Stop capturing any new GameEvents from the engine.");
	ImGui::SameLine();

	{
		std::lock_guard<std::mutex> lock(g_pState->TimelineMutex);
		ImGui::TextDisabled("| Events: %zu", g_pState->Timeline.size());
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Total number of events currently stored in memory.");
		}
	}

	ImGui::SameLine();
	ImGui::TextDisabled("| Processed: %zu", g_pState->ProcessedCount.load());
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

			{
				std::lock_guard lock(g_pState->TimelineMutex);
				const auto& timeline = g_pState->Timeline;

				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(timeline.size()));

				while (clipper.Step())
				{
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
					{
						const auto& gameEvent = timeline[i];
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