#include "pch.h"
#include "Utils/Formatting.h"
#include "Core/Common/GlobalState.h"
#include "Core/UserInterface/Tabs/Primary/DirectorTab.h"
#include "External/imgui/imgui.h"

static void DrawDirectorSystemStatus()
{
	bool init = g_pState->directorInitialized.load();
	bool hooks = g_pState->directorHooksReady.load();

	ImGui::AlignTextToFramePadding();
	ImGui::TextDisabled("System Status:");
	ImGui::SameLine();

	ImGui::Text("Initialized:");
	ImGui::SameLine();
	ImGui::TextColored(init ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), init ? "READY" : "OFFLINE");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Director logic and script engine status.");
	}

	ImGui::SameLine(0.0f, 20.0f);

	ImGui::Text("Engine Hooks:");
	ImGui::SameLine();
	ImGui::TextColored(hooks ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), hooks ? "ACTIVE" : "MISSING");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Direct memory hooks into the game engine for camera/speed control.");
	}
}

static void DrawDirectorProgress(bool autoScroll)
{
	float lastTime = g_pState->lastReplayTime.load();
	size_t currentIndex = g_pState->currentCommandIndex.load();
	size_t totalCommands = 0;

	{
		std::lock_guard<std::mutex> lock(g_pState->directorMutex);
		totalCommands = g_pState->script.size();
	}

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Playback Time: %s", Formatting::ToTimestamp(lastTime).c_str());

	ImGui::SameLine(0.0f, 40.0f);

	float progress = totalCommands > 0 ? (float)currentIndex / (float)totalCommands : 0.0f;
	ImGui::Text("Script Progress:");
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
	ImGui::ProgressBar(progress, ImVec2(200, 0), "");
	ImGui::PopStyleColor();

	ImGui::SameLine();
	ImGui::Text("%zu / %zu Commands", currentIndex, totalCommands);

	ImGui::SameLine(ImGui::GetWindowWidth() - 300);
	ImGui::Checkbox("Auto-Scroll to Current Command", &autoScroll);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Follow the current executing command in real-time.");
	}
}

void DirectorTab::Draw()
{
	static bool autoScroll = true;

	DrawDirectorSystemStatus();
	ImGui::Separator();

	DrawDirectorProgress(autoScroll);
	ImGui::Separator();

	ImGui::TextDisabled("GENERATED SCRIPT COMMANDS");

	if (ImGui::BeginChild("DirectorScriptRegion", ImVec2(0.0f, 0.0f), true))
	{
		static ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

		if (ImGui::BeginTable("DirectorScriptTable", 5, tableFlags))
		{
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Target Player", ImGuiTableColumnFlags_WidthFixed, 180.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 70.0f);
			ImGui::TableSetupColumn("Reason", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

			std::lock_guard<std::mutex> lock(g_pState->directorMutex);
			const auto& script = g_pState->script;
			size_t currentIndex = g_pState->currentCommandIndex.load();

			for (size_t i = 0; i < script.size(); i++)
			{
				const auto& cmd = script[i];
				ImGui::TableNextRow();

				bool isCurrent = (i == (currentIndex > 0 ? currentIndex - 1 : 0) && currentIndex > 0);
				if (isCurrent)
				{
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.2f, 0.4f, 0.6f, 0.5f)));
					if (autoScroll) ImGui::SetScrollHereY(0.5f);
				}

				// Column: Timestamp
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(Formatting::ToTimestamp(cmd.Timestamp).c_str());

				// Column: Type
				ImGui::TableSetColumnIndex(1);
				if (cmd.Type == CommandType::Cut)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "CUT TO");
				}
				else if (cmd.Type == CommandType::SetSpeed)
				{
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "SPEED");
				}

				// Column: Target
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%s [%d]", cmd.TargetPlayerName.c_str(), cmd.TargetPlayerIdx);

				// Column: Value
				ImGui::TableSetColumnIndex(3);
				if (cmd.Type == CommandType::SetSpeed) ImGui::Text("%.2fx", cmd.SpeedValue);
				else                                   ImGui::TextDisabled("-");

				// Column: Reason
				ImGui::TableSetColumnIndex(4);
				ImGui::TextUnformatted(cmd.Reason.c_str());
			}

			ImGui::EndTable();
		}
	}

	ImGui::EndChild();
}