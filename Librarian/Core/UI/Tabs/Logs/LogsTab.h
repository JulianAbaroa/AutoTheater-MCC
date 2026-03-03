#pragma once

#include <atomic>

class LogsTab
{
public:
	void Draw();
	
private:
	void DrawSearchBar(char* buffer, size_t bufferSize);

	std::atomic<bool> m_AutoScroll{ true };

	const float m_AnimationDuration = 0.8f;
	float m_AnimationStartTime = 0.0f;
	int m_AnimateIndex = -1;

    int m_SelectionStart = -1;
    int m_SelectionEnd = -1;

    bool IsIndexSelected(int index) const 
    {
        if (m_SelectionStart == -1 || m_SelectionEnd == -1) return false;
        int minIdx = (std::min)(m_SelectionStart, m_SelectionEnd);
        int maxIdx = (std::max)(m_SelectionStart, m_SelectionEnd);
        return (index >= minIdx && index <= maxIdx);
    }
};