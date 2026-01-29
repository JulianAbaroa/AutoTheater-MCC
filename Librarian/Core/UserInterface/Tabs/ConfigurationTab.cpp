#include "pch.h"
#include "Core/Common/GlobalState.h"
#include "Core/Common/PersistenceManager.h"
#include "Core/UserInterface/Tabs/ConfigurationTab.h"
#include "External/imgui/imgui.h"

void ConfigurationTab::Draw()
{
	ImGui::TextDisabled("USER INTERFACE");

	ImGui::BulletText("Toggle Menu:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "CTRL + 1");

	ImGui::Spacing();

	ImGui::BulletText("Emergency Reset (centers the window):");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "CTRL + 2");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Recover the window if it goes off-screen.");
	}

	ImGui::Spacing();

	bool blockMouse = g_pState->freezeMouse.load();
	if (ImGui::Checkbox("Freeze Mouse Input", &blockMouse))
	{
		g_pState->freezeMouse.store(blockMouse);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(
			"Prevents the game camera from moving while the Control Panel is open.\n"
			"Useful for precise UI interaction during Theater mode."
		);
	}

	ImGui::Spacing();

	bool useAppData = g_pState->useAppData.load();

	bool checkVal = useAppData;
	if (ImGui::Checkbox("Use AppData", &checkVal))
	{
		if (!checkVal)
		{
			ImGui::OpenPopup("Confirm Disable AppData");
		}
		else
		{
			g_pState->useAppData.store(true);
			PersistenceManager::CreateAppData();
			PersistenceManager::SavePreferences();
		}
	}

	if (ImGui::BeginPopupModal("Confirm Disable AppData", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Are you sure you want to disable local storage?\n\n"
			"All custom event weights, saved timelines, and\n"
			"preferences will be lost.");
		ImGui::Separator();
		ImGui::Spacing();

		float widthBtn = 120.0f;
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		float totalButtonWidth = (widthBtn * 2) + spacing;

		float posX = (ImGui::GetContentRegionAvail().x - totalButtonWidth) * 0.5f;
		if (posX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + posX);

		if (ImGui::Button("Yes", ImVec2(widthBtn, 0)))
		{
			g_pState->useAppData.store(false);
			PersistenceManager::SavePreferences();
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(widthBtn, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Allows AutoTheater to save your custom event weights, \ntimelines, and preferences permanently to disk.");
	}

	ImGui::SameLine();

	if (ImGui::Button("Delete AppData"))
	{
		PersistenceManager::DeleteAppData();
	}

	ImGui::Spacing();

	static float menuAlpha = 1.0f;
	if (ImGui::SliderFloat("Menu Opacity", &menuAlpha, 0.2f, 1.0f, "%.2f"))
	{
		if (menuAlpha < 0.2f) menuAlpha = 0.2f;

		ImGui::GetStyle().Alpha = menuAlpha;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Adjust the overall transparency. The minimum is 0.20 to avoid invisibility.");
	}

	ImGui::Spacing();

	ImGui::TextDisabled("FILESYSTEM PATHS");

	{
		std::lock_guard lock(g_pState->configMutex);

		auto PathField = [](const char* label, const std::string& path) {
			ImGui::Text(label);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

			ImGui::InputText(std::string("##").append(label).c_str(),
				(char*)path.c_str(), path.size(), ImGuiInputTextFlags_ReadOnly
			);

			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Right-click to copy path");
				if (ImGui::IsMouseClicked(1))
				{
					ImGui::SetClipboardText(path.c_str());
				}
			}

			ImGui::PopStyleColor();
			ImGui::Spacing();
		};

		PathField("Base", g_pState->baseDirectory);
		PathField("Logger", g_pState->loggerPath);
		PathField("AppData", g_pState->appDataDirectory);
		PathField("MCC Temp", g_pState->mccTempDirectory);
	}
}