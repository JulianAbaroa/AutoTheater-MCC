#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Director/DirectorState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Director/DirectorSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/FormatSystem.h"
#include "Core/UI/Tabs/Primary/DirectorTab.h"
#include "External/imgui/imgui.h"

void DirectorTab::Draw()
{
	this->DrawDirectorSystemStatus();

	ImGui::Separator();

	bool autoScroll = g_pState->Infrastructure->Settings->GetDirectorAutoScroll();
	this->DrawDirectorProgress(autoScroll);

	if (ImGui::BeginChild("DirectorScriptRegion", ImVec2(0.0f, 0.0f), true))
	{
		if (ImGui::BeginTable("DirectorScriptTable", 5, m_TableFlags))
		{
			ImVec4 tabActiveColor = ImGui::GetStyleColorVec4(ImGuiCol_TabActive);
			ImVec4 rowBgAlt = ImVec4(tabActiveColor.x, tabActiveColor.y, tabActiveColor.z, 0.05f);

			ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, tabActiveColor);
			ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, rowBgAlt);

			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, tabActiveColor);
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, tabActiveColor);

			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 100.0f);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 80.0f);
			ImGui::TableSetupColumn("Target Player", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 180.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 70.0f);
			ImGui::TableSetupColumn("Reason", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort);
			ImGui::TableHeadersRow();

			const auto& script = g_pState->Domain->Director->GetScriptCopy();
			size_t currentIndex = g_pSystem->Domain->Director->GetCurrentCommandIndex();

			for (size_t i = 0; i < script.size(); i++)
			{
				const auto& cmd = script[i];
				ImGui::TableNextRow();

				bool isCurrent = (i == (currentIndex > 0 ? currentIndex - 1 : 0) && currentIndex > 0);
				if (isCurrent)
				{
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.2f, 0.4f, 0.6f, 0.5f)));
					if (g_pState->Infrastructure->Settings->GetDirectorAutoScroll()) ImGui::SetScrollHereY(0.5f);
				}

				// Column: Timestamp
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(g_pSystem->Infrastructure->Format->ToTimestamp(cmd.Timestamp).c_str());

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

			ImGui::PopStyleColor(4);

			ImGui::EndTable();
		}
	}

	ImGui::EndChild();
}

void DirectorTab::DrawDirectorSystemStatus()
{
	bool init = g_pState->Domain->Director->IsInitialized();
	bool hooks = g_pState->Domain->Director->AreHooksReady();

	ImGui::AlignTextToFramePadding();
	ImGui::TextDisabled("SYSTEM STATUS");
	ImGui::SameLine();

	ImGui::Text("Initialized:");
	ImGui::SameLine();

	if (init) ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "YES");
	else ImGui::TextDisabled("NO");

	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Director logic and script engine status.");

	ImGui::SameLine(0.0f, 20.0f);

	ImGui::Text("Engine Hooks:");
	ImGui::SameLine();

	if (hooks) ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "HOOKED");
	else ImGui::TextDisabled("MISSING"); 

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Direct memory hooks into the game engine for camera/speed control.");
	}
}

void DirectorTab::DrawDirectorProgress(bool& autoScroll)
{
	float lastTime = g_pSystem->Domain->Director->GetLastReplayTime();
	size_t currentIndex = g_pSystem->Domain->Director->GetCurrentCommandIndex();
	size_t totalCommands = g_pState->Domain->Director->GetScriptSize();

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Playback Time: %s", g_pSystem->Infrastructure->Format->ToTimestamp(lastTime).c_str());

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

	if (ImGui::Checkbox("Auto-Scroll to Current Command", &autoScroll))
	{
		g_pState->Infrastructure->Settings->SetDirectorAutoScroll(autoScroll);
	}
}