#pragma once

#include "Core/Common/Types/TimelineTypes.h"
#include <atomic>

class TimelineTab
{
public:
	void Draw();

private:
	void DrawTimelineControls(bool& autoScroll);
	void RenderPlayerCell(const std::vector<PlayerInfo>& players);

	std::atomic<bool> m_AutoScroll{ true };
	ImGuiTableFlags m_TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit |
		ImGuiTableFlags_RowBg;
};