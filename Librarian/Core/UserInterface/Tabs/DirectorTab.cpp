#include "pch.h"
#include "Core/Common/GlobalState.h"
#include "Core/UserInterface/Tabs/DirectorTab.h"
#include "External/imgui/imgui.h"

void DirectorTab::Draw()
{
	static bool autoScroll = true;

	bool init = g_pState->directorInitialized.load();
	bool hooks = g_pState->directorHooksReady.load();

	ImGui::Checkbox("Auto-Scroll", &autoScroll);

	ImGui::SameLine();
	ImGui::Text(" | ");
	ImGui::SameLine();

	ImGui::Text("Initialized:"); 
	ImGui::SameLine();
	ImGui::TextColored(init ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), init ? "YES" : "NO");

	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();

	ImGui::Text("Hooks Ready:");
	ImGui::SameLine();
	ImGui::TextColored(hooks ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), hooks ? "YES" : "NO");

	ImGui::Separator();

	float lastTime = g_pState->lastReplayTime.load();
	size_t currentIndex = g_pState->currentCommandIndex.load();
	size_t totalCommands = 0;

	{
		std::lock_guard lock(g_pState->directorMutex);
		totalCommands = g_pState->script.size();
	}

	int h = (int)lastTime / 3600;
	int m = ((int)lastTime % 3600) / 60;
	int s = (int)lastTime % 60;

	ImGui::Text("Last Replay Time: %02d:%02d:%02d", h, m, s);
	ImGui::SameLine();
	ImGui::Text("Progress: %zu / %zu", currentIndex, totalCommands);

	ImGui::Spacing();
	ImGui::Separator();

	ImGui::TextDisabled("GENERATED SCRIPT COMMANDS");

	if (ImGui::BeginChild("DirectorScriptRegion", ImVec2(0, 0), true))
	{
		static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

		if (ImGui::BeginTable("DirectorScriptTable", 5, flags))
		{
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Target", ImGuiTableColumnFlags_WidthFixed, 180.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Reason", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableHeadersRow();

			std::lock_guard lock(g_pState->directorMutex);

			for (size_t i = 0; i < g_pState->script.size(); i++)
			{
				const auto& cmd = g_pState->script[i];

				ImGui::TableNextRow();

				bool isCurrentRow = (i == currentIndex - 1);

				if (isCurrentRow)
				{
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.0f, 0.6f)));

					if (autoScroll)
					{
						ImGui::SetScrollHereY(0.5f);
					}
				}

				ImGui::TableSetColumnIndex(0);
				int th = (int)cmd.Timestamp / 3600;
				int tm = ((int)cmd.Timestamp % 3600) / 60;
				int ts = (int)cmd.Timestamp % 60;
				if (th > 0) ImGui::Text("%02d:%02d:%02d", th, tm, ts);
				else ImGui::Text("%02d:%02d", tm, ts);

				ImGui::TableSetColumnIndex(1);
				if (cmd.Type == CommandType::Cut)
				{
					ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "CUT");
				}
				else 
				{
					ImGui::TextColored(ImVec4(0.5f, 1, 0.5f, 1), "SPEED");
				}

				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%s [%d]", cmd.TargetPlayerName.c_str(), cmd.TargetPlayerIdx);

				ImGui::TableSetColumnIndex(3);
				if (cmd.Type == CommandType::SetSpeed)
				{
					ImGui::Text("%.2fx", cmd.SpeedValue);
				}
				else
				{
					ImGui::Text("-");
				}

				ImGui::TableSetColumnIndex(4);
				ImGui::TextUnformatted(cmd.Reason.c_str());
			}

			ImGui::EndTable();
		}
	}

	ImGui::EndChild();
}