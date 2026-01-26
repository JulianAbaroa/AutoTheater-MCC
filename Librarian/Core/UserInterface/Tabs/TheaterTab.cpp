#include "pch.h"
#include "Core/Systems/Theater.h"
#include "Core/Common/GlobalState.h"
#include "Core/UserInterface/Tabs/TheaterTab.h"
#include "External/imgui/imgui_internal.h"
#include "External/imgui/imgui.h"

void TheaterTab::Draw()
{
	static bool autoScroll = false;

	bool active = g_pState->isTheaterMode.load();
	if (active)
	{
		ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "THEATER ACTIVE");
	}
	else
	{
		ImGui::TextDisabled("THEATER INACTIVE");
	}

	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();

	bool attachPOV = g_pState->attachThirdPersonPOV.load();
	if (ImGui::Checkbox("Lock Third-Person POV (for Director Phase)", &attachPOV))
	{
		g_pState->attachThirdPersonPOV.store(attachPOV);
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("It forces the camera into third person when attached to a player.");
	}

	ImGui::Separator();

	float* pTime = g_pState->pReplayTime.load();

	if (pTime)
	{
		int h = (int)(*pTime) / 3600;
		int m = ((int)(*pTime) % 3600) / 60;
		int s = (int)(*pTime) % 60;
		ImGui::Text("Time: %02d:%02d:%02d", h, m, s);
	}
	else
	{
		ImGui::TextDisabled("Time: --:--:--");
	}

	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();

	float* pScale = g_pState->pReplayTimeScale.load();
	float realScale = g_pState->realTimeScale.load();

	if (pScale)
	{
		ImGui::BeginGroup();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Speed: %.2fx (Real: %.2fx)", *pScale, realScale);
		ImGui::SameLine();

		float tempSpeed = *pScale;
		ImGui::PushItemWidth(200.0f);
		if (ImGui::SliderFloat("##SpeedSlider", &tempSpeed, 0.0f, 16.0f, "Target: %.2fx"))
		{
			if (tempSpeed < 0.0f) tempSpeed = 0.0f;
			if (tempSpeed > 24.0f) tempSpeed = 24.0f;

			Theater::SetReplaySpeed(tempSpeed);
		}
		ImGui::PopItemWidth();
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Ctrl + Click to type a custom value (Max: 24x).");
		}

		ImGui::SameLine();

		if (*pScale > 1.0f && realScale < (*pScale * 0.85f))
		{
			ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "[System Bottleneck]");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("The game engine cannot process the replay at this speed.\nThis is usually limited by Disk I/O, CPU or Blam! itself.");
			}
		}

		ImGui::EndGroup();
	}
	else
	{
		ImGui::TextDisabled("Speed: N/A");
	}

	ImGui::Separator();

	ImGui::TextDisabled("PLAYER LIST");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("It is only updated during the Timeline Phase");
	}

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 150);
	ImGui::Checkbox("Auto-Scroll", &autoScroll);

	if (ImGui::BeginChild("PlayerListRegion", ImVec2(0, 0), true))
	{
		static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

		if (ImGui::BeginTable("TheaterPlayerTable", 3, flags))
		{
			ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30.0f);
			ImGui::TableSetupColumn("Name [Tag]", ImGuiTableColumnFlags_WidthFixed, 200.0f);
			ImGui::TableSetupColumn("Last Event Position (X, Y, Z)", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableHeadersRow();

			std::lock_guard lock(g_pState->theaterMutex);

			uint8_t followed = g_pState->followedPlayerIdx.load();

			for (const auto& player : g_pState->playerList)
			{
				if (player.Name.empty()) continue;

				ImGui::TableNextRow();

				bool isFollowed = player.Id == followed;
				if (isFollowed)
				{
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.0f, 0.6f)));
				}

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%d", player.Id);

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s [%s]", player.Name.c_str(), player.Tag.c_str());

				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%.2f, %.2f, %.2f",
					player.RawPlayer.Position[0],
					player.RawPlayer.Position[1],
					player.RawPlayer.Position[2]
				);

				if (isFollowed && autoScroll)
				{
					ImGui::SetScrollHereY(0.5f);
				}
			}

			ImGui::EndTable();
		}
	}

	ImGui::EndChild();
}