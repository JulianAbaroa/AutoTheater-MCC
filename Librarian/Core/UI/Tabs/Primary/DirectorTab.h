#pragma once

#include "Core/Common/Types/DirectorTypes.h"
#include "External/imgui/imgui.h"
#include <atomic>
#include <vector>

class DirectorTab
{
public:
	void Draw();

	void ResetCachedScript();

private:
	void DrawDirectorSystemStatus();
	void DrawDirectorProgress(bool& autoScroll);

	ImGuiTableFlags m_TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

	std::vector<DirectorCommand> m_CachedScript{};
	bool m_ScriptCached = false;
};