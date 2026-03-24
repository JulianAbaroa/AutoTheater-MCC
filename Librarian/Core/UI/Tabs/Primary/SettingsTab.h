#pragma once

#include "External/imgui/imgui.h"
#include <atomic>
#include <string>

class SettingsTab
{
public:
	void Draw();

	void DrawPathField(const char* label, const std::string& path, float widthOffset = 10.0f);

private:
	void DrawUserPreferences();
	void DrawHotkeysTable();
	void DrawDataPersistence();
	void DrawSystemDirectories();

	void DrawHotkeyRow(const char* label, const char* keys, const char* tooltip);

	void DrawPersistencePopups();
	void DrawConfirmDisableAppData();
	void DrawDeleteAllAppData();

	std::string GetFriendlyGameName(const std::string& path);

	ImGuiTableFlags m_TableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoBordersInBody;

	std::string m_AnimatePathLabel = "";
	float m_AnimationStartTime = 0.0f;
	const float m_AnimationDuration = 0.6f;

	float m_UIScalePreview = 1.0f;
	bool m_IsInitialized = false;
};