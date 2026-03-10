#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Persistence/ReplayState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Persistence/ReplaySystem.h"
#include "Core/UI/Tabs/Optional/ReplayManagerTab.h"
#include "External/imgui/imgui.h" 

void ReplayManagerTab::Draw()
{
    if (ImGui::BeginTabBar("ReplayManagerTabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Replay Library"))
        {
            this->DrawLibrary();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("In-Game Replays"))
        {
            this->DrawInGameReplays();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    // Popups
    this->DrawDeleteLibraryReplay();
    this->DrawDeleteGameReplay();
}

void ReplayManagerTab::DrawLibrary()
{
    DrawSearchBar("##search_inlibrary", m_LibrarySearchBuffer, IM_ARRAYSIZE(m_LibrarySearchBuffer));
    ImGui::Spacing();

    this->RefreshLibraryCache();
    std::vector<SavedReplay> savedReplaysCopy = g_pState->Infrastructure->Replay->GetSavedReplaysCacheCopy();

    std::string searchStr = m_LibrarySearchBuffer;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    int visibleCount = this->GetVisibleLibraryCount(savedReplaysCopy, searchStr);
    ImGui::AlignTextToFramePadding();

    if (savedReplaysCopy.empty()) ImGui::TextDisabled("Library is empty...");
    else ImGui::TextDisabled("Showing %d saved replays", visibleCount);

    float buttonWidth = ImGui::CalcTextSize("Refresh List").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - buttonWidth);

    if (ImGui::Button("Refresh List"))
    {
        g_pState->Infrastructure->Replay->SetRefreshReplayList(true);
        m_SelectedLibIndex = -1;
    }

    ImGui::Separator();

    if (ImGui::BeginChild("ReplayList", ImVec2(0, 0), ImGuiChildFlags_Borders))
    {
        if (!savedReplaysCopy.empty())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12.0f, 8.0f));

            for (int i = 0; i < (int)savedReplaysCopy.size(); i++)
            {
                this->DrawLibraryReplayRow(i, savedReplaysCopy[i], searchStr);
            }

            ImGui::PopStyleVar();
        }
        
        ImGui::EndChild();
    }
}

void ReplayManagerTab::RefreshLibraryCache()
{
    if (g_pState->Infrastructure->Replay->ShouldRefreshReplayList())
    {
        auto data = g_pSystem->Infrastructure->Replay->GetSavedReplays();
        g_pState->Infrastructure->Replay->SetSavedReplaysCache(data);
        g_pState->Infrastructure->Replay->SetRefreshReplayList(false);
    }
}

bool ReplayManagerTab::MatchesLibraryFilter(const SavedReplay& replay, const std::string& filter)
{
    if (filter.empty()) return true;

    auto ToLower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    };

    return
        ToLower(replay.DisplayName).find(filter) != std::string::npos ||
        ToLower(replay.TheaterReplay.ReplayMetadata.Author).find(filter) != std::string::npos ||
        ToLower(replay.TheaterReplay.ReplayMetadata.Info).find(filter) != std::string::npos;
}

int ReplayManagerTab::GetVisibleLibraryCount(const std::vector<SavedReplay>& replays, const std::string& filter)
{
    if (filter.empty()) return (int)replays.size();

    int count = 0;

    for (const auto& r : replays) 
    {
        if (MatchesLibraryFilter(r, filter)) count++;
    }

    return count;
}

void ReplayManagerTab::DrawLibraryReplayRow(int index, SavedReplay& replay, const std::string& filter)
{
    if (!MatchesLibraryFilter(replay, filter)) return;

    ImGui::PushID(replay.Hash.c_str());
    bool isSelected = (m_SelectedLibIndex == index);

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, isSelected ? 2.0f : 1.0f);

    if (isSelected) 
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.20f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
    }
    else 
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyleColorVec4(ImGuiCol_Border));
    }

    if (ImGui::BeginChild("ReplayItem", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY))
    {
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) m_SelectedLibIndex = index;

        if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(0)) 
        {
            m_EditingIndex = index;
            strncpy_s(m_RenameBuffer, replay.DisplayName.c_str(), sizeof(m_RenameBuffer));
        }

        if (ImGui::BeginTable("ReplayItemTable", 2, ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch, 0.65f);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch, 0.35f);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            if (m_EditingIndex == index) this->DrawRenameInput(index, replay);
            else ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", replay.DisplayName.c_str());

            ImGui::TextDisabled("Author: %s", replay.TheaterReplay.ReplayMetadata.Author.c_str());
            ImGui::TextWrapped("%s", replay.TheaterReplay.ReplayMetadata.Info.empty() ? "No info" : replay.TheaterReplay.ReplayMetadata.Info.c_str());

            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button("Restore to Game", ImVec2(-FLT_MIN, 0)))
            {
                g_pSystem->Infrastructure->Replay->RestoreReplay(replay);
            }

            if (!replay.HasTimeline) ImGui::BeginDisabled();
            if (ImGui::Button("Load Timeline", ImVec2(-FLT_MIN, 0)))
            {
                g_pSystem->Infrastructure->Replay->LoadTimeline(replay.Hash);
            }
            if (!replay.HasTimeline) ImGui::EndDisabled();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
            if (ImGui::Button("Delete", ImVec2(-FLT_MIN, 0))) 
            {
                m_HashToDelete = replay.Hash;
                m_OpenLibDeleteModal = true;
            }
            ImGui::PopStyleColor();

            ImGui::EndTable();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::PopID();
    ImGui::Spacing();
}

void ReplayManagerTab::DrawRenameInput(int index, SavedReplay& replay)
{
    ImGui::SetNextItemWidth(-1);
    ImGui::SetKeyboardFocusHere();

    if (ImGui::InputText("##rename_input", m_RenameBuffer, IM_ARRAYSIZE(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        g_pSystem->Infrastructure->Replay->RenameReplay(replay.Hash, m_RenameBuffer);
        g_pState->Infrastructure->Replay->SetRefreshReplayList(true);
        m_EditingIndex = -1;
    }

    if (ImGui::IsItemDeactivated() && !ImGui::IsItemActive())
    {
        m_EditingIndex = -1;
    }
}


void ReplayManagerTab::DrawInGameReplays()
{
    DrawSearchBar("##search_ingame", m_InGameSearchBuffer, IM_ARRAYSIZE(m_InGameSearchBuffer));
    ImGui::Spacing();

    this->RefreshInGameCache();

    std::string searchStr = m_InGameSearchBuffer;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    int visibleCount = GetVisibleInGameCount(searchStr);
    ImGui::AlignTextToFramePadding();

    if (m_CachedInGameReplays.empty()) ImGui::TextDisabled("No active replays in game directory.");
    else ImGui::TextDisabled("Showing %d active replays", visibleCount);

    float buttonWidth = ImGui::CalcTextSize("Refresh List").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - buttonWidth);

    if (ImGui::Button("Refresh List"))
    {
        m_NeedsInGameRefresh = true;
        m_SelectedGameIndex = -1;
    }

    ImGui::Separator();

    if (m_CachedInGameReplays.empty()) return;

    if (ImGui::BeginChild("InGameList", ImVec2(0, 0), ImGuiChildFlags_Borders))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12.0f, 8.0f));

        for (int i = 0; i < (int)m_CachedInGameReplays.size(); i++)
        {
            this->DrawInGameReplayRow(i, m_CachedInGameReplays[i], searchStr);
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();
    }
}

void ReplayManagerTab::RefreshInGameCache()
{
    if (!m_NeedsInGameRefresh) return;

    std::filesystem::path tempDir = g_pState->Infrastructure->Settings->GetMovieTempDirectory();
    m_CachedInGameReplays = g_pSystem->Infrastructure->Replay->GetTheaterReplays(tempDir);

    std::erase_if(m_CachedInGameReplays, [](const TheaterReplay& r) {
        return r.MovFileName == ".hotreload_trigger.mov";
    });

    std::sort(m_CachedInGameReplays.begin(), m_CachedInGameReplays.end(), [](const TheaterReplay& a, const TheaterReplay& b) {
        return std::filesystem::last_write_time(a.FullPath) > std::filesystem::last_write_time(b.FullPath);
    });

    m_NeedsInGameRefresh = false;
}

bool ReplayManagerTab::MatchesInGameFilter(const TheaterReplay& replay, const std::string& filter)
{
    if (filter.empty()) return true;

    auto ToLower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    };

    return 
        ToLower(replay.MovFileName).find(filter) != std::string::npos ||
        ToLower(replay.ReplayMetadata.Author).find(filter) != std::string::npos ||
        ToLower(replay.ReplayMetadata.Info).find(filter) != std::string::npos;
}

int ReplayManagerTab::GetVisibleInGameCount(const std::string& filter)
{
    if (filter.empty()) return (int)m_CachedInGameReplays.size();

    int count = 0;

    for (const auto& replay : m_CachedInGameReplays) 
    {
        if (MatchesInGameFilter(replay, filter)) count++;
    }

    return count;
}

void ReplayManagerTab::DrawInGameReplayRow(int index, const TheaterReplay& replay, const std::string& filter)
{
    if (!MatchesInGameFilter(replay, filter)) return;

    bool isSelected = (m_SelectedGameIndex == index);
    ImGui::PushID(index);

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, isSelected ? 2.0f : 1.0f);

    if (isSelected) 
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.20f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
    }
    else 
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyleColorVec4(ImGuiCol_Border));
    }

    if (ImGui::BeginChild("IngameItem", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY))
    {
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) m_SelectedGameIndex = index;

        if (ImGui::BeginTable("IngameTable", 2, ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch, 0.65f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.35f);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(isSelected ? ImVec4(1, 0.8f, 0.2f, 1) : ImVec4(0.4f, 0.8f, 1, 1), "%s", replay.MovFileName.c_str());
            ImGui::TextDisabled("By: %s", replay.ReplayMetadata.Author.empty() ? "Unknown" : replay.ReplayMetadata.Author.c_str());

            if (!replay.ReplayMetadata.Info.empty())
            {
                ImGui::TextWrapped("%s", replay.ReplayMetadata.Info.c_str());
            }

            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button("Save to Replay Library", ImVec2(-FLT_MIN, 0))) 
            {
                g_pSystem->Infrastructure->Replay->SaveReplay(replay.FullPath.string());
            }

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Delete", ImVec2(-FLT_MIN, 0))) 
            {
                m_PathToDelete = replay.FullPath;
                m_OpenGameDeleteModal = true;
            }
            ImGui::PopStyleColor();
            ImGui::EndTable();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::PopID();
    ImGui::Spacing();
}


void ReplayManagerTab::DrawDeleteLibraryReplay()
{
    if (m_OpenLibDeleteModal)
    {
        ImGui::OpenPopup("Delete Library Replay");
        m_OpenLibDeleteModal = false;
    }

    if (ImGui::BeginPopupModal("Delete Library Replay", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("This replay will be permanently removed from your library.\nThis action cannot be undone.");
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = 120.0f;
        float totalButtonsWidth = (buttonWidth * 2.0f) + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalButtonsWidth) * 0.5f);

        if (ImGui::Button("Yes", ImVec2(120, 0)))
        {
            g_pSystem->Infrastructure->Replay->DeleteReplay(m_HashToDelete);
            g_pState->Infrastructure->Replay->SetRefreshReplayList(true);
            m_SelectedLibIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void ReplayManagerTab::DrawDeleteGameReplay()
{
    if (m_OpenGameDeleteModal)
    {
        ImGui::OpenPopup("Delete Game Replay");
        m_OpenGameDeleteModal = false;
    }

    if (ImGui::BeginPopupModal("Delete Game Replay", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("This replay will be permanently removed from the game directory.\nThis action cannot be undone.");
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = 120.0f;
        float totalButtonsWidth = (buttonWidth * 2.0f) + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalButtonsWidth) * 0.5f);

        if (ImGui::Button("Yes", ImVec2(120, 0)))
        {
            g_pSystem->Infrastructure->Replay->DeleteInGameReplay(m_PathToDelete);
            m_NeedsInGameRefresh = true;
            if (m_SelectedGameIndex >= 0) m_SelectedGameIndex = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}


void ReplayManagerTab::DrawSearchBar(const char* label, char* buffer, size_t bufferSize)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Filter:");
    ImGui::SameLine();

    float clearBtnWidth = 60.0f;
    float availableWidth = ImGui::GetContentRegionAvail().x;

    ImGui::PushItemWidth(availableWidth - clearBtnWidth - ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::InputTextWithHint(label, "Search for specific names, maps or authors...", buffer, bufferSize)) {}
    ImGui::PopItemWidth();
    ImGui::SameLine();

    if (ImGui::Button("Clear", ImVec2(clearBtnWidth, 0)))
    {
        buffer[0] = '\0';
    }
}