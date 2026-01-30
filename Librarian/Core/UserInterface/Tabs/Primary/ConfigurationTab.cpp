#include "pch.h"
#include "Core/Common/GlobalState.h"
#include "Core/Common/PersistenceManager.h"
#include "Core/UserInterface/Tabs/Primary/ConfigurationTab.h"
#include "External/imgui/imgui.h"

static void DrawHotkey(const char* label, const char* keys, const char* tooltip)
{
	ImGui::BulletText("%s", label);
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "%s", keys);
	if (tooltip && ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", tooltip);
	}
}

static void DrawPathField(const char* label, const std::string& path)
{
	ImGui::Text("%s", label);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));

	std::string id = "##Path_" + std::string(label);
	ImGui::InputText(id.c_str(), (char*)path.c_str(), path.size(), ImGuiInputTextFlags_ReadOnly);

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Right-click to copy this path to clipboard.");
		if (ImGui::IsMouseClicked(1))
		{
			ImGui::SetClipboardText(path.c_str());
		}
	}

	ImGui::PopStyleColor();
	ImGui::Spacing();
}

void ConfigurationTab::Draw()
{
	// Section: User Interface
	ImGui::TextDisabled("USER INTERFACE & HOTKEYS");
	ImGui::Separator();
	ImGui::Spacing();

	DrawHotkey("Toggle Menu", "CTRL + 1", "Show or hide the AutoTheater Control Panel.");
	DrawHotkey("Emergency Reset", "CTRL + 2", "Centers the window if it gets lost off-screen.");

	ImGui::Spacing();

	static float menuAlpha = 1.0f;
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Menu Alpha");
	ImGui::SameLine();
	ImGui::PushItemWidth(200.0f);
	if (ImGui::SliderFloat("##Global Opacity", &menuAlpha, 0.2f, 1.0f, "%.2f"))
	{
		ImGui::GetStyle().Alpha = (std::max)(menuAlpha, 0.20f);
	}
	ImGui::PopItemWidth();

	ImGui::Spacing();

	bool blockMouse = g_pState->freezeMouse.load();
	if (ImGui::Checkbox("Freeze Mouse Input", &blockMouse))
	{
		g_pState->freezeMouse.store(blockMouse);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Blocks game camera movement while the menu is open.");
	}

	ImGui::Spacing();
	ImGui::Separator();

	// Section: Persistence
	ImGui::TextDisabled("DATA PERSISTENCE");
	ImGui::Spacing();

	bool useAppData = g_pState->useAppData.load();
	if (ImGui::Checkbox("Enable Local Storage (AppData)", &useAppData))
	{
		if (!useAppData)
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
		ImGui::Text("Warning: Disabling local storage will prevent AutoTheater from\nsaving preferences and using Replay Manager/Event Registry.");
		ImGui::Separator();

		float buttonWidth = 120.0f;
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		float totalButtonsWidth = (buttonWidth * 2.0f) + spacing;

		float posX = (ImGui::GetContentRegionAvail().x - totalButtonsWidth) * 0.5f;
		if (posX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + posX);

		if (ImGui::Button("Yes", ImVec2(buttonWidth, 0.0f)))
		{
			g_pState->useAppData.store(false);
			PersistenceManager::SavePreferences();
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::SameLine(0.0f, 30.0f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));

	if (ImGui::Button("Delete All AppData")) {
		ImGui::OpenPopup("CRITICAL: Wipe AppData?");
	}
	ImGui::PopStyleColor(2);

	if (ImGui::BeginPopupModal("CRITICAL: Wipe AppData?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "WARNING: THIS ACTION CANNOT BE UNDONE");
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text(
			"This will permanently delete:\n"
			" - All saved Replays."
			" - All saved Timelines.\n"
			" - Custom Event Registry.\n"
			" - All user preferences."
		);

		ImGui::Spacing();
		ImGui::Text("Are you sure?");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		float btnWidth = 120.0f;
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		float totalWidth = (btnWidth * 2.0f) + spacing;

		float offset = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
		if (offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
		if (ImGui::Button("Confirm Wipe", ImVec2(btnWidth, 0.0f)))
		{
			PersistenceManager::DeleteAppData();
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		if (ImGui::Button("Keep Data", ImVec2(btnWidth, 0.0f)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::Spacing();
	ImGui::Separator();

	ImGui::TextDisabled("SYSTEM DIRECTORIES");
	ImGui::Spacing();

	{
		std::lock_guard<std::mutex> lock(g_pState->configMutex);
		ImGui::Indent(10.0f);
		DrawPathField("Base Installation", g_pState->baseDirectory);
		DrawPathField("Log File Output", g_pState->loggerPath);
		DrawPathField("Storage Folder", g_pState->appDataDirectory);
		DrawPathField("MCC Temporary Movies", g_pState->mccTempMovieDirectory);
		ImGui::Unindent(10.0f);
	}
}