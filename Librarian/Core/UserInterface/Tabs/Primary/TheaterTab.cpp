#include "pch.h"
#include "Utils/Formatting.h"
#include "Core/Common/AppCore.h"
#include "Core/Systems/Domain/TheaterSystem.h"
#include "Core/UserInterface/Tabs/Primary/TheaterTab.h"
#include "External/imgui/imgui_internal.h"
#include "External/imgui/imgui.h"

static void DrawTheaterStatus()
{
	bool active = g_pState->Theater.IsTheaterMode();
	ImGui::AlignTextToFramePadding();
	if (active) ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "THEATER ACTIVE");
	else		ImGui::TextDisabled("THEATER INACTIVE");

	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();

	bool attachToPOV = g_pState->Theater.IsThirdPersonForced();
	if (ImGui::Checkbox("Lock Third-Person POV (Director Phase)", &attachToPOV))
	{
		g_pState->Theater.SetThirdPersonForced(attachToPOV);
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Synchronizes both camera position AND rotation (view direction) with the followed player.");
	}
}

static void DrawPlaybackControls(bool& autoScroll)
{
	float* pTime = g_pState->Theater.GetTimePtr();

	ImGui::AlignTextToFramePadding();
	if (pTime) ImGui::Text("Time: %s", Formatting::ToTimestamp(*pTime).c_str());
	else       ImGui::TextDisabled("Time: --:--:--.--");

	ImGui::SameLine(0, 30.0f);

	float* pScale = g_pState->Theater.GetTimeScalePtr();
	float realScale = g_pSystem->Theater.GetRealTimeScale();

	if (pScale)
	{
		ImGui::Text("Speed: %.2fx", *pScale);
		ImGui::SameLine();

		ImGui::TextDisabled("(Real: %.2fx)", realScale);
		ImGui::SameLine();

		ImGui::PushItemWidth(200.0f);
		float tempSpeed = *pScale;
		if (ImGui::SliderFloat("##Speed", &tempSpeed, 0.0f, 16.0f, "Target: %.2fx"))
		{
			g_pSystem->Theater.SetReplaySpeed(std::clamp(tempSpeed, 0.0f, 24.0f));
		}
		ImGui::PopItemWidth();

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Ctrl + Click to type a custom value (Max: 24x).");

		if (*pScale > 1.0f && realScale < (*pScale * 0.85f))
		{
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "[!] System Bottleneck");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("The engine cannot keep up with this speed.\nLimited by Disk I/O, CPU or Blam! limits.");
		}
	}
	else
	{
		ImGui::TextDisabled("Speed: N/A");
	}

	ImGui::SameLine(ImGui::GetWindowWidth() - 225);
	ImGui::Checkbox("Auto-Scroll to Target", &autoScroll);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Automatically scrolls the list to keep the followed player in view.");
	}
}

void TheaterTab::Draw()
{
	static bool autoScroll = false;

	DrawTheaterStatus();
	ImGui::Separator();

	DrawPlaybackControls(autoScroll);
	ImGui::Separator();

	// Section: Player List
	ImGui::AlignTextToFramePadding();
	ImGui::TextDisabled("PLAYER LIST");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Data snapshot from the last event processed.");
	}

	if (ImGui::BeginChild("PlayerListRegion", ImVec2(0, 0), true))
	{
		static ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

		if (ImGui::BeginTable("TheaterPlayerTable", 3, tableFlags))
		{
			ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 35.0f);
			ImGui::TableSetupColumn("Player [Tag]", ImGuiTableColumnFlags_WidthFixed, 180.0f);
			ImGui::TableSetupColumn("Last Known World Position (X, Y, Z)", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

			uint8_t followedIdx = g_pState->Theater.GetSpectatedPlayerIndex();

			g_pState->Theater.ForEachPlayer([&](const PlayerInfo& player)
			{
				if (player.Name.empty()) return;
			
				ImGui::TableNextRow();
				bool isFollowed = (player.Id == followedIdx);
			
				if (isFollowed)
				{
					ImGui::TableSetBgColor(
						ImGuiTableBgTarget_RowBg0,
						ImGui::GetColorU32(ImVec4(0.4f, 0.4f, 0.1f, 0.5f))
					);
					if (autoScroll) ImGui::SetScrollHereY(0.5f);
				}
			
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%d", player.Id);
			
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s [%s]", player.Name.c_str(), player.Tag.c_str());
			
				ImGui::TableSetColumnIndex(2);
				ImGui::Text(
					"%.3f, %.3f, %.3f",
					player.RawPlayer.Position[0],
					player.RawPlayer.Position[1],
					player.RawPlayer.Position[2]
				);
			});

			ImGui::EndTable();
		}
	}

	ImGui::EndChild();
}