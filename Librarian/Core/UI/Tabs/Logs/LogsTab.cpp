#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/UI/Tabs/Logs/LogsTab.h"
#include "External/imgui/imgui.h"

void LogsTab::Draw()
{
    // --- Botones de Control ---
    if (ImGui::Button("Clear Logs")) g_pState->Debug.ClearLogs();
    ImGui::SameLine();

    if (ImGui::Button("Copy All")) 
    {
        std::string allLogs;
        g_pState->Debug.ForEachLog([&](const LogEntry& entry) { allLogs += entry.FullText + "\n"; });
        ImGui::SetClipboardText(allLogs.c_str());
    }
    ImGui::SameLine();

    bool autoScroll = m_AutoScroll.load();
    if (ImGui::Checkbox("Auto-Scroll", &autoScroll)) m_AutoScroll.store(autoScroll);

    // TODO: Poner a lo mas a la derecha, y mas grande.
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);

        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Log Controls:");
        ImGui::Separator();

        ImGui::BulletText("Left Click: Select a single line.");
        ImGui::BulletText("Shift + Left Click: Select a range of lines.");
        ImGui::BulletText("Right Click: Copy selection (or single line).");

        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    // --- Search Bar ---
    static char searchBuffer[128] = "";
    DrawSearchBar(searchBuffer, IM_ARRAYSIZE(searchBuffer));
    std::string searchStr = searchBuffer;
    bool isFiltering = !searchStr.empty();
    if (isFiltering) std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    ImGui::Separator();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    std::vector<int> filteredIndices;
    int totalLogs = static_cast<int>(g_pState->Debug.GetTotalLogs());
    if (isFiltering) 
    {
        for (int i = 0; i < totalLogs; i++) 
        {
            LogEntry entry = g_pState->Debug.GetLogAt(i);
            std::string textLower = entry.FullText;
            std::transform(textLower.begin(), textLower.end(), textLower.begin(), ::tolower);
            if (textLower.find(searchStr) != std::string::npos) filteredIndices.push_back(i);
        }
    }

    int displayCount = isFiltering ? static_cast<int>(filteredIndices.size()) : totalLogs;
    ImGuiListClipper clipper;
    clipper.Begin(displayCount);

    bool logClickedThisFrame = false;

    while (clipper.Step()) 
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) 
        {
            int realIndex = isFiltering ? filteredIndices[i] : i;
            LogEntry entry = g_pState->Debug.GetLogAt(realIndex);

            ImGui::PushID(realIndex);

            if (IsIndexSelected(realIndex)) 
            {
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetCursorScreenPos(),
                    ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x, ImGui::GetCursorScreenPos().y + ImGui::GetTextLineHeightWithSpacing()),
                    IM_COL32(66, 150, 250, 60)
                );
            }

            ImGui::BeginGroup();

            ImGui::TextDisabled("%s", entry.Timestamp.c_str());
            ImGui::SameLine(0.0f, 0.0f);

            if (!entry.Tag.empty()) 
            {
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "%s", entry.Tag.c_str());
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::TextUnformatted(" ");
                ImGui::SameLine(0.0f, 0.0f);
            }

            if (!entry.MessagePrefix.empty()) 
            {
                ImVec4 prefColor = (entry.Level == LogLevel::Error) ? ImVec4(1.0f, 0.33f, 0.33f, 1.0f) :
                    (entry.Level == LogLevel::Warning) ? ImVec4(1.0f, 0.79f, 0.23f, 1.0f) :
                    (entry.Level == LogLevel::Info) ? ImVec4(0.4f, 0.8f, 0.4f, 1.0f) : ImVec4(1, 1, 1, 1);
                ImGui::TextColored(prefColor, "%s", entry.MessagePrefix.c_str());
                ImGui::SameLine(0.0f, 0.0f);
            }

            const std::string& msg = entry.Message;
            size_t qStart = msg.find('\'');
            size_t qEnd = (qStart != std::string::npos) ? msg.find('\'', qStart + 1) : std::string::npos;
            if (qStart != std::string::npos && qEnd != std::string::npos) 
            {
                if (qStart > 0) { ImGui::TextUnformatted(msg.substr(0, qStart).c_str()); ImGui::SameLine(0.0f, 0.0f); }
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", msg.substr(qStart, qEnd - qStart + 1).c_str());
                if (qEnd + 1 < msg.length()) { ImGui::SameLine(0.0f, 0.0f); ImGui::TextUnformatted(msg.substr(qEnd + 1).c_str()); }
            }
            else 
            {
                ImGui::TextUnformatted(msg.c_str());
            }

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));

            ImGui::EndGroup();

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) 
            {
                logClickedThisFrame = true;

                if (ImGui::GetIO().KeyShift && m_SelectionStart != -1) 
                {
                    m_SelectionEnd = realIndex;
                }
                else 
                {
                    m_SelectionStart = realIndex;
                    m_SelectionEnd = realIndex;
                }
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) 
            {
                if (IsIndexSelected(realIndex)) 
                {
                    std::string selectedText;
                    int start = (std::min)(m_SelectionStart, m_SelectionEnd);
                    int end = (std::max)(m_SelectionStart, m_SelectionEnd);
                    for (int j = start; j <= end; j++) selectedText += g_pState->Debug.GetLogAt(j).FullText + "\n";
                    ImGui::SetClipboardText(selectedText.c_str());
                    m_AnimateIndex = -2;
                }
                else 
                {
                    ImGui::SetClipboardText(entry.FullText.c_str());
                    m_AnimateIndex = realIndex;
                }

                m_AnimationStartTime = (float)ImGui::GetTime();
            }

            if (m_AnimateIndex == realIndex || (m_AnimateIndex == -2 && IsIndexSelected(realIndex))) 
            {
                float elapsed = (float)ImGui::GetTime() - m_AnimationStartTime;
                if (elapsed < m_AnimationDuration) 
                {
                    float alpha = 1.0f - (elapsed / m_AnimationDuration);
                    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(1.0f, 1.0f, 1.0f, alpha * 0.3f));
                }
                else if (m_AnimateIndex != -2) 
                {
                    m_AnimateIndex = -1;
                }
            }

            ImGui::PopID();
        }
    }

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && !logClickedThisFrame) 
    {
        m_SelectionStart = m_SelectionEnd = -1;
    }

    if (m_AutoScroll.load() && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
}

void LogsTab::DrawSearchBar(char* buffer, size_t bufferSize)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Filter:");
    ImGui::SameLine();

    float clearBtnWidth = 60.0f;
    float availableWidth = ImGui::GetContentRegionAvail().x;

    ImGui::PushItemWidth(availableWidth - clearBtnWidth - ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::InputTextWithHint("##log_filter", "Search specific logs...", buffer, bufferSize)) { }
    ImGui::PopItemWidth();
    ImGui::SameLine();

    if (ImGui::Button("Clear", ImVec2(clearBtnWidth, 0)))
    {
        buffer[0] = '\0';
    }
}