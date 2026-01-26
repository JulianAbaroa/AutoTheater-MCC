#include "pch.h"
#include "Core/Common/GlobalState.h"
#include "Core/UserInterface/Tabs/LogsTab.h"
#include "External/imgui/imgui.h"

void LogsTab::Draw()
{
	bool static autoScroll = true;

	if (ImGui::Button("Clear Logs"))
	{
		std::lock_guard lock(g_pState->logMutex);
		g_pState->debugLogs.clear();
	} 

	ImGui::SameLine();
	ImGui::Checkbox("Auto-Scroll", &autoScroll);

	ImGui::Separator();

	const float FOOTER_HEIGHT_TO_RESERVE = 
		ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

	ImGui::BeginChild("ScrollingRegion", ImVec2(0, -FOOTER_HEIGHT_TO_RESERVE), false, ImGuiWindowFlags_HorizontalScrollbar);

	{
		std::lock_guard lock(g_pState->logMutex);

		for (const auto& log : g_pState->debugLogs)
		{
			if (log.find("ERROR") != std::string::npos)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), log.c_str());
			}
			else
			{
				ImGui::TextUnformatted(log.c_str());
			}

			if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			{
				ImGui::SetScrollHereY(1.0f);
			}
		}

		ImGui::EndChild();
	}
}