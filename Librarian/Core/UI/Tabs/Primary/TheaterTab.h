#pragma once

#include "Core/Common/Types/TimelineTypes.h"
#include "Core/Common/Types/TheaterTypes.h"
#include "External/imgui/imgui.h"
#include <atomic>
#include <vector>

class TheaterTab
{
public:
	void Draw();

	void SetPlayerListDirty();

private:
	void DrawTheaterStatus(ReplayModule replayModule);
	void DrawPlaybackControls(bool& autoScroll);

	ImGuiTableFlags m_TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

	std::vector<PlayerInfo> m_CachedPlayerList{};
	std::atomic<bool> m_PlayerListDirty{ false };
};