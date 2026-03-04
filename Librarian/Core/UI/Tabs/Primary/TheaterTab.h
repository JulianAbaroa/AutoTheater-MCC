#pragma once

#include <atomic>

class TheaterTab
{
public:
	void Draw();

private:
	void DrawTheaterStatus();
	void DrawPlaybackControls(bool& autoScroll);

	std::atomic<bool> m_AutoScroll{ true };
	ImGuiTableFlags m_TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
};