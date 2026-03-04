#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/UI/Tabs/Logs/LogsTab.h"
#include "External/imgui/imgui.h"

void LogsTab::Draw()
{
    auto filterState = this->DrawTopBar();
    ImGui::Separator();
    this->DrawScrollingRegion(filterState);
}


LogFilterState LogsTab::DrawTopBar()
{
    DrawSearchBar(m_SearchBuffer, IM_ARRAYSIZE(m_SearchBuffer));

    std::string searchStr = m_SearchBuffer;

    bool isFiltering = !searchStr.empty();
    if (isFiltering)
    {
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);
    }

    this->DrawClearButton(isFiltering, searchStr);

    ImGui::SameLine();

    this->DrawCopyButton(isFiltering, searchStr);

    ImGui::SameLine();

    bool autoScroll = m_AutoScroll.load();
    if (ImGui::Checkbox("Auto-Scroll", &autoScroll)) m_AutoScroll.store(autoScroll);

    ImGui::SameLine();

    this->DrawHelpMarker();

    return LogFilterState{ searchStr, isFiltering };
}

void LogsTab::DrawClearButton(bool isFiltering, std::string& searchStr)
{
    const char* clearBtnLabel = isFiltering ? "Clear Filtered" : "Clear All Logs";

    if (ImGui::Button(clearBtnLabel))
    {
        if (isFiltering)
        {
            std::string lowerFilter = searchStr;
            g_pSystem->Debug.RemoveLogsIf([&](const LogEntry& entry) {
                std::string textLower = entry.FullText;
                std::transform(textLower.begin(), textLower.end(), textLower.begin(), ::tolower);
                return textLower.find(lowerFilter) != std::string::npos;
                });
        }
        else
        {
            g_pState->Debug.ClearLogs();
        }

        // Reset selections.
        m_SelectionStart.store(-1);
        m_SelectionEnd.store(-1);
    }
}

void LogsTab::DrawCopyButton(bool isFiltering, std::string& searchStr)
{
    const char* copyBtnLabel = isFiltering ? "Copy Filtered" : "Copy All";

    if (ImGui::Button(copyBtnLabel))
    {
        std::string output;

        g_pState->Debug.ForEachLog([&](const LogEntry& entry) {
            if (isFiltering)
            {
                std::string textLower = entry.FullText;
                std::transform(textLower.begin(), textLower.end(), textLower.begin(), ::tolower);

                if (textLower.find(searchStr) != std::string::npos)
                {
                    output += entry.FullText + "\n";
                }
            }
            else
            {
                output += entry.FullText + "\n";
            }
        });

        ImGui::SetClipboardText(output.c_str());
    }
}

void LogsTab::DrawHelpMarker()
{
    float helpIconWidth = ImGui::CalcTextSize("(?)").x;
    float posX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - helpIconWidth;
    ImGui::SetCursorPosX(posX);

    ImGui::TextDisabled("(?)");

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Log Controls:");
        ImGui::Separator();
        ImGui::BulletText("Buttons: Behavior changes based on current filter.");
        ImGui::BulletText("Left Click: Select a single line.");
        ImGui::BulletText("Shift + Left Click: Select a range.");
        ImGui::BulletText("Right Click: Copy selection or single line.");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}


std::vector<int> LogsTab::GetFilteredIndices(const LogFilterState& filter)
{
    std::vector<int> filtered;
    if (!filter.IsFiltering) return filtered;

    int totalLogs = static_cast<int>(g_pState->Debug.GetTotalLogs());
    filtered.reserve(totalLogs / 2);

    for (int i = 0; i < totalLogs; i++)
    {
        LogEntry entry = g_pState->Debug.GetLogAt(i);
        std::string textLower = entry.FullText;
        
        std::transform(textLower.begin(), textLower.end(), textLower.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (textLower.find(filter.SearchStr) != std::string::npos)
        {
            filtered.push_back(i);
        }
    }

    return filtered;
}

void LogsTab::DrawScrollingRegion(const LogFilterState& filter)
{
    if (!ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::EndChild();
        return;
    }

    auto filteredIndices = this->GetFilteredIndices(filter);
    int totalLogs = static_cast<int>(g_pState->Debug.GetTotalLogs());
    int displayCount = filter.IsFiltering ? (int)filteredIndices.size() : totalLogs;

    ImGuiListClipper clipper;
    clipper.Begin(displayCount);

    bool logClickedThisFrame = false;

    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            int realIndex = filter.IsFiltering ? filteredIndices[i] : i;
            LogEntry entry = g_pState->Debug.GetLogAt(realIndex);

            this->DrawLogLine(realIndex, entry, logClickedThisFrame);
        }
    }

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && !logClickedThisFrame)
    {
        m_SelectionStart.store(-1);
        m_SelectionEnd.store(-1);
    }

    if (m_AutoScroll.load() && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

void LogsTab::DrawLogLine(int realIndex, const LogEntry& entry, bool& logClickedThisFrame)
{
    ImGui::PushID(realIndex);

    if (this->IsIndexSelected(realIndex))
    {
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetCursorScreenPos(),
            ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x,
                ImGui::GetCursorScreenPos().y + ImGui::GetTextLineHeightWithSpacing()),
            IM_COL32(66, 150, 250, 60));
    }

    ImGui::BeginGroup();

    ImGui::TextDisabled("%s", entry.Timestamp.c_str());
    ImGui::SameLine(0.0f, 0.0f);

    if (!entry.Tag.empty()) 
    {
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), " %s ", entry.Tag.c_str());
        ImGui::SameLine(0.0f, 0.0f);
    }

    if (!entry.MessagePrefix.empty()) 
    {
        ImVec4 prefColor = (entry.Level == LogLevel::Error) ? 
            ImVec4(1.0f, 0.33f, 0.33f, 1.0f) : (entry.Level == LogLevel::Warning) ? 
            ImVec4(1.0f, 0.79f, 0.23f, 1.0f) : (entry.Level == LogLevel::Info) ? 
            ImVec4(0.4f, 0.8f, 0.4f, 1.0f) : ImVec4(1, 1, 1, 1);

        ImGui::TextColored(prefColor, "%s", entry.MessagePrefix.c_str());
        ImGui::SameLine(0.0f, 0.0f);
    }

    this->DrawLogMessage(entry.Message);

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
    ImGui::EndGroup();

    this->HandleLogInteraction(realIndex, entry, logClickedThisFrame);

    ImGui::PopID();
}

void LogsTab::DrawLogMessage(const std::string& message)
{
    size_t qStart = message.find('\'');
    size_t qEnd = (qStart != std::string::npos) ? message.find('\'', qStart + 1) : std::string::npos;

    if (qStart != std::string::npos && qEnd != std::string::npos)
    {
        if (qStart > 0) 
        {
            ImGui::TextUnformatted(message.substr(0, qStart).c_str());
            ImGui::SameLine(0.0f, 0.0f);
        }

        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s",
            message.substr(qStart, qEnd - qStart + 1).c_str());

        if (qEnd + 1 < message.length()) 
        {
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::TextUnformatted(message.substr(qEnd + 1).c_str());
        }
    }
    else
    {
        ImGui::TextUnformatted(message.c_str());
    }
}

void LogsTab::HandleLogInteraction(int realIndex, const LogEntry& entry, bool& logClickedThisFrame)
{
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        logClickedThisFrame = true;

        if (ImGui::GetIO().KeyShift && m_SelectionStart.load() != -1)
        {
            m_SelectionEnd.store(realIndex);
        }
        else
        {
            m_SelectionStart.store(realIndex);
            m_SelectionEnd.store(realIndex);
        }
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        if (this->IsIndexSelected(realIndex))
        {
            std::string selectedText;
            int start = (std::min)(m_SelectionStart.load(), m_SelectionEnd.load());
            int end = (std::max)(m_SelectionStart.load(), m_SelectionEnd.load());

            for (int j = start; j <= end; j++)
            {
                selectedText += g_pState->Debug.GetLogAt(j).FullText + "\n";
            }

            ImGui::SetClipboardText(selectedText.c_str());
            m_AnimateIndex.store(-2);
        }
        else
        {
            ImGui::SetClipboardText(entry.FullText.c_str());
            m_AnimateIndex.store(realIndex);
        }
        m_AnimationStartTime.store((float)ImGui::GetTime());
    }

    bool isAnimatingSelection = (m_AnimateIndex.load() == -2 && this->IsIndexSelected(realIndex));
    if (m_AnimateIndex.load() == realIndex || isAnimatingSelection)
    {
        float elapsed = (float)ImGui::GetTime() - m_AnimationStartTime.load();
        if (elapsed < m_AnimationDuration)
        {
            float alpha = 1.0f - (elapsed / m_AnimationDuration);

            ImGui::GetWindowDrawList()->AddRectFilled(
                ImGui::GetItemRectMin(),
                ImGui::GetItemRectMax(),
                ImColor(1.0f, 1.0f, 1.0f, alpha * 0.3f));
        }
        else if (m_AnimateIndex.load() != -2)
        {
            m_AnimateIndex.store(-1);
        }
    }
}


void LogsTab::DrawSearchBar(char* buffer, size_t bufferSize)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Filter:");
    ImGui::SameLine();

    float clearBtnWidth = 60.0f;
    float availableWidth = ImGui::GetContentRegionAvail().x;

    ImGui::PushItemWidth(availableWidth - clearBtnWidth - ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::InputTextWithHint("##log_filter", "Search for specific logs...", buffer, bufferSize)) { }
    ImGui::PopItemWidth();
    ImGui::SameLine();

    if (ImGui::Button("Clear", ImVec2(clearBtnWidth, 0)))
    {
        buffer[0] = '\0';
    }
}

bool LogsTab::IsIndexSelected(int index) const
{
    if (m_SelectionStart.load() == -1 || m_SelectionEnd.load() == -1) return false;

    int minIdx = (std::min)(m_SelectionStart.load(), m_SelectionEnd.load());
    int maxIdx = (std::max)(m_SelectionStart.load(), m_SelectionEnd.load());

    return (index >= minIdx && index <= maxIdx);
}