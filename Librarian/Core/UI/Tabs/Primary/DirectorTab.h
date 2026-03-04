#pragma once

#include <atomic>

class DirectorTab
{
public:
	void Draw();

private:
	void DrawDirectorSystemStatus();
	void DrawDirectorProgress(bool& autoScroll);

	std::atomic<bool> m_AutoScroll{ true };
	ImGuiTableFlags m_TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
};