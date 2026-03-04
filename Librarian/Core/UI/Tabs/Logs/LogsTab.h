#pragma once

#include "Core/Common/Types/UITypes.h"
#include "Core/Common/Types/LogTypes.h"
#include <atomic>

class LogsTab
{
public:
	void Draw();
	
private:
	LogFilterState DrawTopBar();
	void DrawClearButton(bool isFiltering, std::string& searchStr);
	void DrawCopyButton(bool isFiltering, std::string& searchStr);
	void DrawHelpMarker();

	std::vector<int> GetFilteredIndices(const LogFilterState& filter);
	void DrawScrollingRegion(const LogFilterState& filter);
	void DrawLogLine(int realIndex, const LogEntry& entry, bool& logClickedThisFrame);
	void DrawLogMessage(const std::string& message);
	void HandleLogInteraction(int realIndex, const LogEntry& entry, bool& logClickedThisFrame);

	void DrawSearchBar(char* buffer, size_t bufferSize);
	bool IsIndexSelected(int index) const;

	char m_SearchBuffer[128] = "";

	std::atomic<int> m_SelectionEnd{ -1 };
	std::atomic<int> m_SelectionStart{ -1 };
	std::atomic<float> m_AnimationStartTime{ 0.0f };
	std::atomic<int> m_AnimateIndex{ -1 };
	const float m_AnimationDuration = 0.8f;
};