#include "pch.h"
#include "Core/Common/GlobalState.h"
#include "Core/UserInterface/Tabs/Logs/LogsTab.h"
#include "External/imgui/imgui.h"
#include <vector>
#include <string>
#include <algorithm>

static void DrawSearchBar(char* buffer, size_t bufferSize)
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

void LogsTab::Draw()
{
    static bool autoScroll = true;

    if (ImGui::Button("Clear Logs"))
    {
        std::lock_guard lock(g_pState->LogMutex);
        g_pState->DebugLogs.clear();
    }

    ImGui::SameLine();
    if (ImGui::Button("Copy to Clipboard"))
    {
        std::string allLogs;
        std::lock_guard lock(g_pState->LogMutex);
        for (const auto& log : g_pState->DebugLogs) allLogs += log + "\n";
        ImGui::SetClipboardText(allLogs.c_str());
    }

    ImGui::SameLine();
    ImGui::Checkbox("Auto-Scroll", &autoScroll);

    static char searchBuffer[128] = "";
    DrawSearchBar(searchBuffer, IM_ARRAYSIZE(searchBuffer));

    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    {
        std::lock_guard lock(g_pState->LogMutex);

        std::string searchStr = searchBuffer;
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

        for (const auto& log : g_pState->DebugLogs)
        {
            if (!searchStr.empty())
            {
                std::string logLower = log;
                std::transform(logLower.begin(), logLower.end(), logLower.begin(), ::tolower);
                if (logLower.find(searchStr) == std::string::npos) continue;
            }

            ImVec4 logColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            bool customColor = false;

            if (log.find("ERROR") != std::string::npos || log.find("Error") != std::string::npos)
            {
                logColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                customColor = true;
            }
            else if (log.find("WARNING") != std::string::npos || log.find("Warning") != std::string::npos)
            {
                logColor = ImVec4(1.0f, 0.7f, 0.0f, 1.0f);
                customColor = true;
            }
            else if (log.find("INFO") != std::string::npos)
            {
                logColor = ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
                customColor = true;
            }

            if (customColor)
                ImGui::TextColored(logColor, "%s", log.c_str());
            else
                ImGui::TextUnformatted(log.c_str());
        }

        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }
    }

    ImGui::EndChild();
}