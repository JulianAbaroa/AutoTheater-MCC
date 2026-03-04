#pragma once

#include <atomic>

class SettingsTab
{
public:
	void Draw();

private:
	void DrawUserPreferences();
	void DrawHotkeysTable();
	void DrawDataPersistence();
	void DrawSystemDirectories();

	void DrawHotkeyRow(const char* label, const char* keys, const char* tooltip);
	void DrawPathField(const char* label, const std::string& path);

	void DrawPersistencePopups();
	void DrawConfirmDisableAppData();
	void DrawDeleteAllAppData();

	std::atomic<float> m_MenuAlpha{ 1.0f };
	ImGuiTableFlags m_TableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoBordersInBody;

	std::string m_AnimatePathLabel = "";
	float m_AnimationStartTime = 0.0f;
	const float m_AnimationDuration = 0.6f;
};