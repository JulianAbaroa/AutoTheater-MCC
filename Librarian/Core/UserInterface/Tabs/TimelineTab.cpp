#include "pch.h"
#include "Utils/Formatting.h"
#include "External/imgui/imgui.h"
#include "Core/Common/GlobalState.h"
#include "Core/UserInterface/Tabs/TimelineTab.h"
#include <vector>
#include <string>

static std::string FormatPlayers(const std::vector<PlayerInfo>& players)
{
	if (players.empty()) return "-";

	std::string result;

	for (size_t i = 0; i < players.size(); ++i)
	{
		result += players[i].Name + " [" + players[i].Tag + "]";
		if (i < players.size() - 1) result += ", ";
	}

	return result;
}

void TimelineTab::Draw()
{
	static bool autoScroll = true;
	static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

	ImGui::Checkbox("Auto-scroll", &autoScroll);
	ImGui::SameLine();

	bool logEnabled = g_pState->logGameEvents.load();
	if (ImGui::Checkbox("Log GameEvents", &logEnabled))
	{
		g_pState->logGameEvents.store(logEnabled);
	}
	ImGui::SameLine();

	bool lastEvent = g_pState->isLastEvent.load();
	if (ImGui::Checkbox("Last GameEvent", &lastEvent))
	{
		g_pState->isLastEvent.store(lastEvent);
	}
	ImGui::SameLine();

	{
		std::lock_guard<std::mutex> lock(g_pState->timelineMutex);
		ImGui::TextDisabled("|  Events: %zu", g_pState->timeline.size());
	}
	ImGui::SameLine();

	ImGui::TextDisabled("| Processed: %zu", g_pState->processedCount.load());

	ImGui::Separator();

	if (ImGui::BeginChild("TimelineContent", ImVec2(0, 0), false))
	{
		if (ImGui::BeginTable("TimelineTable", 3, flags))
		{
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 200.0f);
			ImGui::TableSetupColumn("Event Type", ImGuiTableColumnFlags_WidthFixed, 300.0f);
			ImGui::TableSetupColumn("Involved Players", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

			{
				std::lock_guard lock(g_pState->timelineMutex);

				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(g_pState->timeline.size()));

				while (clipper.Step())
				{
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
					{
						const auto& ev = g_pState->timeline[i];
						ImGui::TableNextRow();

						ImGui::TableSetColumnIndex(0);
						int h = (int)ev.Timestamp / 3600;
						int m = ((int)ev.Timestamp % 3600) / 60;
						int s = (int)ev.Timestamp % 60;
						if (h > 0) ImGui::Text("%02d:%02d:%02d", h, m, s);
						else ImGui::Text("%02d:%02d", m, s);

						ImGui::TableSetColumnIndex(1);
						ImGui::TextUnformatted(Formatting::EventTypeToString(ev.Type).c_str());

						ImGui::TableSetColumnIndex(2);
						ImGui::TextUnformatted(FormatPlayers(ev.Players).c_str());
					}
				}

				if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				{
					ImGui::SetScrollHereY(1.0f);
				}
			}

			ImGui::EndTable();
		}
	}

	ImGui::EndChild();
}