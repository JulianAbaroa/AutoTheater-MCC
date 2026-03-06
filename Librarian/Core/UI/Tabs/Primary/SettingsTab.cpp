#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/UI/Tabs/Primary/SettingsTab.h"
#include "External/imgui/imgui.h"

void SettingsTab::Draw()
{
	if (ImGui::BeginChild("SettingsScroll", ImVec2(0, 0), false))
	{
		if (ImGui::CollapsingHeader("User Preferences"))
		{
			ImGui::Indent(10.0f);

			this->DrawUserPreferences();

			ImGui::Unindent(10.0f);
			ImGui::Spacing();
		}

		if (ImGui::CollapsingHeader("Hotkeys", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10.0f);

			this->DrawHotkeysTable();

			ImGui::Unindent(10.0f);
			ImGui::Spacing();
		}

		if (ImGui::CollapsingHeader("Data Persistence"))
		{
			ImGui::Indent(10.0f);

			this->DrawDataPersistence();

			ImGui::Unindent(10.0f);
			ImGui::Spacing();
		}

		if (ImGui::CollapsingHeader("System Directories"))
		{
			ImGui::Indent(10.0f);

			this->DrawSystemDirectories();

			ImGui::Unindent(10.0f);
		}

		ImGui::EndChild();
	}
}

void SettingsTab::DrawUserPreferences()
{
	if (!m_IsInitialized)
	{
		m_UIScalePreview = g_pState->Render.GetUIScale();
		m_IsInitialized = true;
	}

	ImGui::BeginGroup();
	ImVec2 p_min = ImGui::GetCursorScreenPos();

	ImGui::Spacing();
	ImGui::Indent(10.0f);
	ImGui::Spacing();

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Menu Opacity");

	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 205.0f);
	ImGui::PushItemWidth(200.0f);

	float menuAlpha = g_pState->Settings.GetMenuAlpha();
	if (ImGui::SliderFloat("##Global Opacity", &menuAlpha, 0.2f, 1.0f, "%.2f"))
	{
		ImGui::GetStyle().Alpha = (std::max)(menuAlpha, 0.20f);
		g_pState->Settings.SetMenuAlpha(menuAlpha);
	}
	ImGui::PopItemWidth();

	ImGui::Spacing();

	ImGui::AlignTextToFramePadding();
	ImGui::Text("UI Scale");
	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 205.0f);

	ImGui::PushItemWidth(200.0f);
	if (ImGui::SliderFloat("##GlobalScale", &m_UIScalePreview, 1.0f, 4.0f, "%.2f")) {}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		m_UIScalePreview = std::clamp(m_UIScalePreview, 1.0f, 4.0f);
		g_pState->Render.SetUIScale(m_UIScalePreview);

		g_pUtil->Log.Append("[SettingsTab] INFO: Applied scale: %.2f", m_UIScalePreview);
	}
	ImGui::PopItemWidth();

	ImGui::Spacing();

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Preferred Phase:");
	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 205.0f);
	ImGui::SetNextItemWidth(205.0f);

	AutoTheaterPhase prefPhase = g_pState->Settings.GetPreferredPhase();
	const char* phaseNames[] = { "Default", "Timeline", "Director" };
	int currentIdx = (int)prefPhase;

	if (ImGui::BeginCombo("##PreferredPhaseCombo", phaseNames[currentIdx]))
	{
		for (int n = 0; n < IM_ARRAYSIZE(phaseNames); n++)
		{
			bool isSelected = (currentIdx == n);
			if (ImGui::Selectable(phaseNames[n], isSelected))
			{
				g_pState->Settings.SetPreferredPhase((AutoTheaterPhase)n);
			}

			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The phase that will be automatically selected when the application starts.");
	}

	ImGui::Spacing();

	bool blockMouse = g_pState->Settings.ShouldFreezeMouse();
	if (ImGui::Checkbox("Freeze Mouse Input", &blockMouse))
	{
		g_pState->Settings.SetFreezeMouse(blockMouse);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Blocks game camera movement while the menu is open.");
	}

	ImGui::Spacing();

	bool useManualInput = g_pState->Settings.ShouldUseManualInput();
	if (ImGui::Checkbox("Replay speed modifier keys", &useManualInput))
	{
		g_pState->Settings.SetUseManualInput(useManualInput);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Allows to use number keys (3-0) to change the replay speed.");
	}

	ImGui::Spacing();
	ImGui::Unindent(10.0f);
	ImGui::Spacing();

	ImGui::EndGroup();

	ImVec2 p_max = ImVec2(p_min.x + ImGui::GetItemRectSize().x + 20.0f, p_min.y + ImGui::GetItemRectSize().y);
	ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, ImColor(255, 255, 255, 15), 5.0f);
	ImGui::GetWindowDrawList()->AddRect(p_min, p_max, ImColor(255, 255, 255, 30), 5.0f);
}

void SettingsTab::DrawHotkeysTable()
{
	ImGui::BeginGroup();
	ImVec2 p_min = ImGui::GetCursorScreenPos();

	ImGui::Spacing();
	ImGui::Indent(10.0f);
	ImGui::Spacing();

	float tableWidth = ImGui::GetContentRegionAvail().x - 10.0f;

	if (ImGui::BeginTable("HotkeysTable", 2, m_TableFlags, ImVec2(tableWidth, 0)))
	{
		ImVec4 tabActiveColor = ImGui::GetStyleColorVec4(ImGuiCol_TabActive);

		ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, tabActiveColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, tabActiveColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, tabActiveColor);

		ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.6f);
		ImGui::TableSetupColumn("Shortcut", ImGuiTableColumnFlags_WidthStretch, 0.4f);

		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
		for (int i = 0; i < 2; i++)
		{
			ImGui::TableSetColumnIndex(i);
			const char* columnName = ImGui::TableGetColumnName(i);
			float textWidth = ImGui::CalcTextSize(columnName).x;
			float columnWidth = ImGui::GetColumnWidth();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f);
			ImGui::TableHeader(columnName);
		}

		this->DrawHotkeyRow("Toggle Menu", "CTRL + 1", "Show or hide the AutoTheater Control Panel.");
		this->DrawHotkeyRow("Emergency Reset", "CTRL + 2", "Centers the window if it gets lost off-screen.");
		this->DrawHotkeyRow("Speed 0.0x", "3", "Set the replay speed to 0.0x");
		this->DrawHotkeyRow("Speed 0.1x", "4", "Set the replay speed to 0.1x");
		this->DrawHotkeyRow("Speed 0.25x", "5", "Set the replay speed to 0.25x");
		this->DrawHotkeyRow("Speed 0.5x", "6", "Set the replay speed to 0.5x");
		this->DrawHotkeyRow("Speed 1.0x", "7", "Set the replay speed to 1.0x");
		this->DrawHotkeyRow("Speed 4.0x", "8", "Set the replay speed to 4.0x");
		this->DrawHotkeyRow("Speed 8.0x", "9", "Set the replay speed to 8.0x");
		this->DrawHotkeyRow("Speed 16.0x", "0", "Set the replay speed to 16.0x");

		ImGui::PopStyleColor(3);
		ImGui::EndTable();
	}

	ImGui::Spacing();
	ImGui::Unindent(10.0f);
	ImGui::Spacing();

	ImGui::EndGroup();

	ImVec2 p_max = ImVec2(p_min.x + ImGui::GetItemRectSize().x + 20.0f, p_min.y + ImGui::GetItemRectSize().y);
	ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, ImColor(255, 255, 255, 15), 5.0f);
	ImGui::GetWindowDrawList()->AddRect(p_min, p_max, ImColor(255, 255, 255, 30), 5.0f);
}

void SettingsTab::DrawDataPersistence()
{
	bool openDisablePopup = false;
	bool openDeletePopup = false;

	ImGui::BeginGroup();

	ImVec2 p_min = ImGui::GetCursorScreenPos();

	ImGui::Spacing();
	ImGui::Indent(10.0f);
	ImGui::Spacing();

	ImGui::BeginGroup();
	bool useAppData = g_pState->Settings.ShouldUseAppData();
	ImGui::AlignTextToFramePadding();
	if (ImGui::Checkbox("Enable Local Storage (AppData)", &useAppData))
	{
		if (!useAppData) openDisablePopup = true;
		else {
			g_pState->Settings.SetUseAppData(true);
			g_pSystem->Settings.CreateAppData();
			g_pSystem->Settings.SaveUseAppData();
			g_pSystem->Preferences.LoadPreferences();
		}
	}
	ImGui::EndGroup();

	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 140.0f);

	ImGui::BeginGroup();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));

	if (ImGui::Button("Delete Data", ImVec2(130, 0)))
		openDeletePopup = true;

	ImGui::PopStyleColor(2);
	ImGui::EndGroup();

	ImGui::Spacing();
	ImGui::Unindent(10.0f);
	ImGui::Spacing();

	ImGui::EndGroup();

	ImVec2 p_max = ImVec2(p_min.x + ImGui::GetItemRectSize().x + 20.0f, p_min.y + ImGui::GetItemRectSize().y);
	ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, ImColor(255, 255, 255, 15), 5.0f);
	ImGui::GetWindowDrawList()->AddRect(p_min, p_max, ImColor(255, 255, 255, 30), 5.0f);

	if (openDisablePopup) ImGui::OpenPopup("Confirm Disable AppData");
	if (openDeletePopup)  ImGui::OpenPopup("Delete AppData");

	this->DrawPersistencePopups();
}

void SettingsTab::DrawSystemDirectories()
{
	ImVec2 p_min = ImGui::GetCursorScreenPos();
	float width = ImGui::GetContentRegionAvail().x;

	ImGui::BeginGroup();

	ImGui::Spacing();
	ImGui::Indent(10.0f);
	ImGui::Spacing();

	this->DrawPathField("Base Installation", g_pState->Settings.GetBaseDirectory());
	this->DrawPathField("Log File Output", g_pState->Settings.GetLoggerPath());
	this->DrawPathField("Storage Folder", g_pState->Settings.GetAppDataDirectory());
	this->DrawPathField("MCC Temporary Movies", g_pState->Settings.GetMovieTempDirectory());

	ImGui::Spacing();
	ImGui::Unindent(10.0f);
	ImGui::Spacing();

	ImGui::EndGroup();

	ImVec2 p_max = ImVec2(p_min.x + width, p_min.y + ImGui::GetItemRectSize().y);

	ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, ImColor(255, 255, 255, 15), 5.0f);
	ImGui::GetWindowDrawList()->AddRect(p_min, p_max, ImColor(255, 255, 255, 30), 5.0f);
}


void SettingsTab::DrawHotkeyRow(const char* label, const char* keys, const char* tooltip)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	float col1Width = ImGui::GetColumnWidth();
	float text1Width = ImGui::CalcTextSize(label).x;

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (col1Width - text1Width) * 0.5f);
	ImGui::TextUnformatted(label);

	if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);

	ImGui::TableNextColumn();

	float col2Width = ImGui::GetColumnWidth();
	float text2Width = ImGui::CalcTextSize(keys).x;

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (col2Width - text2Width) * 0.5f);
	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", keys);
}

void SettingsTab::DrawPathField(const char* label, const std::string& path)
{
	ImGui::BeginGroup();
	ImGui::TextDisabled("%s", label);

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 10.0f);

	ImGui::InputText(("##" + std::string(label)).c_str(), (char*)path.c_str(), path.size(), ImGuiInputTextFlags_ReadOnly);
	ImGui::PopStyleColor();

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Right-click to copy path to clipboard.");

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			ImGui::SetClipboardText(path.c_str());
			m_AnimatePathLabel = label;
			m_AnimationStartTime = (float)ImGui::GetTime();
		}
	}

	if (m_AnimatePathLabel == label)
	{
		float elapsed = (float)ImGui::GetTime() - m_AnimationStartTime;
		if (elapsed < m_AnimationDuration)
		{
			float alpha = 1.0f - (elapsed / m_AnimationDuration);
			
			ImGui::GetWindowDrawList()->AddRectFilled(
				ImGui::GetItemRectMin(),
				ImGui::GetItemRectMax(),
				ImColor(1.0f, 1.0f, 1.0f, alpha * 0.4f), 
				ImGui::GetStyle().FrameRounding
			);
		}
		else
		{
			m_AnimatePathLabel = "";
		}
	}

	ImGui::EndGroup();
	ImGui::Spacing();
}


void SettingsTab::DrawPersistencePopups()
{
	if (ImGui::BeginPopupModal("Confirm Disable AppData", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		this->DrawConfirmDisableAppData();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Delete AppData", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		this->DrawDeleteAllAppData();
		ImGui::EndPopup();
	}
}

void SettingsTab::DrawConfirmDisableAppData()
{
	ImGui::Text("Warning: Disabling local storage will prevent AutoTheater from saving"
		"\npreferences and using Replay Manager, Event Registry & Captures.");

	ImGui::Separator();

	float buttonWidth = 120.0f;
	float spacing = ImGui::GetStyle().ItemSpacing.x;
	float totalButtonsWidth = (buttonWidth * 2.0f) + spacing;
	
	float posX = (ImGui::GetContentRegionAvail().x - totalButtonsWidth) * 0.5f;
	if (posX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + posX);
	
	if (ImGui::Button("Yes", ImVec2(buttonWidth, 0.0f)))
	{
		g_pState->Settings.SetUseAppData(false);
		g_pSystem->Settings.SaveUseAppData();
		ImGui::CloseCurrentPopup();
	}
	
	ImGui::SameLine();
	
	if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)))
	{
		ImGui::CloseCurrentPopup();
	}
}

void SettingsTab::DrawDeleteAllAppData()
{
	ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "WARNING: THIS ACTION CANNOT BE UNDONE");

	ImGui::Separator();

	ImGui::Text(
		"This will permanently delete:\n"
		" - All saved Replays.\n"
		" - All saved Timelines.\n"
		" - Custom Event Registry.\n"
		" - All user preferences.");

	ImGui::Separator();
	ImGui::Spacing();

	float btnWidth = 120.0f;
	float spacing = ImGui::GetStyle().ItemSpacing.x;
	float totalWidth = (btnWidth * 2.0f) + spacing;

	float offset = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
	if (offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

	if (ImGui::Button("Yes", ImVec2(btnWidth, 0.0f)))
	{
		g_pSystem->Settings.DeleteAppData();
		g_pState->Replay.SetRefreshReplayList(true);
		g_pState->Settings.SetUseAppData(false);
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel", ImVec2(btnWidth, 0.0f)))
	{
		ImGui::CloseCurrentPopup();
	}
}