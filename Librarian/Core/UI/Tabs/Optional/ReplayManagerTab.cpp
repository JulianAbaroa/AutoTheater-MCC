#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/UI/Tabs/Optional/ReplayManagerTab.h"
#include "External/imgui/imgui.h" 

// TODO: Test the safety thing of active session when clearing sessions.

void ReplayManagerTab::Draw()
{
    this->DrawCurrentSession();

    ImGui::Spacing(); 

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
    std::vector<SavedReplay> savedReplaysCopy = g_pState->Replay.GetSavedReplaysCacheCopy();

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
        g_pState->Replay.SetRefreshReplayList(true);
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
    if (g_pState->Replay.ShouldRefreshReplayList())
    {
        auto data = g_pSystem->Replay.GetSavedReplays();
        g_pState->Replay.SetSavedReplaysCache(data);
        g_pState->Replay.SetRefreshReplayList(false);
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
        ToLower(replay.TheaterReplay.FilmMetadata.Author).find(filter) != std::string::npos ||
        ToLower(replay.TheaterReplay.FilmMetadata.Info).find(filter) != std::string::npos;
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

            ImGui::TextDisabled("Author: %s", replay.TheaterReplay.FilmMetadata.Author.c_str());
            ImGui::TextWrapped("%s", replay.TheaterReplay.FilmMetadata.Info.empty() ? "No info" : replay.TheaterReplay.FilmMetadata.Info.c_str());

            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button("Restore to Game", ImVec2(-FLT_MIN, 0)))
            {
                g_pSystem->Replay.RestoreReplay(replay);
            }

            if (!replay.HasTimeline) ImGui::BeginDisabled();
            if (ImGui::Button("Load Timeline", ImVec2(-FLT_MIN, 0)))
            {
                g_pSystem->Replay.LoadTimeline(replay.Hash);
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
        g_pSystem->Replay.RenameReplay(replay.Hash, m_RenameBuffer);
        g_pState->Replay.SetRefreshReplayList(true);
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

    std::filesystem::path tempDir = g_pState->Settings.GetMovieTempDirectory();
    m_CachedInGameReplays = g_pSystem->Replay.GetTheaterReplays(tempDir);

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
        ToLower(replay.FilmMetadata.Author).find(filter) != std::string::npos ||
        ToLower(replay.FilmMetadata.Info).find(filter) != std::string::npos;
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
            ImGui::TextDisabled("By: %s", replay.FilmMetadata.Author.empty() ? "Unknown" : replay.FilmMetadata.Author.c_str());

            if (!replay.FilmMetadata.Info.empty())
            {
                ImGui::TextWrapped("%s", replay.FilmMetadata.Info.c_str());
            }

            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button("Save to Replay Library", ImVec2(-FLT_MIN, 0))) 
            {
                g_pSystem->Replay.SaveReplay(replay.FullPath.string());
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


void ReplayManagerTab::DrawCurrentSession()
{
    ImGui::TextDisabled("ACTIVE SESSION (?)");
    if (ImGui::IsItemHovered()) this->DrawActiveSessionTooltip();

    ImGui::Separator();

    std::string currentFilmPath = g_pState->Replay.GetCurrentReplayPath();

    if (!currentFilmPath.empty())
    {
        static std::string cachedHash = "";

        this->UpdateCurrentSessionState(currentFilmPath, cachedHash);

        this->DrawSessionPath(currentFilmPath);

        ImGui::Spacing();

        this->DrawSessionActions(currentFilmPath, cachedHash);
    }
    else
    {
        ImGui::TextDisabled("No active theater film detected. Open a replay in Halo MCC.");
    }
}

void ReplayManagerTab::DrawActiveSessionTooltip()
{
    ImGui::BeginTooltip();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Session Information");
    ImGui::Separator();

    ImGui::Text("AutoTheater automatically detects the .mov file opened by MCC via Hook.");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Safety Mismatch System:");
    ImGui::BulletText("Each film has a unique Hash. The Timeline is tied to this Hash.");
    ImGui::BulletText("If you open a different film while in 'Director Phase',\n"
        "the system will abort to prevent crashes.");

    ImGui::EndTooltip();
}

void ReplayManagerTab::UpdateCurrentSessionState(const std::string& path, std::string& outHash)
{
    std::string lastProcessedPath = g_pState->Replay.GetPreviousReplayPath();

    if (path != lastProcessedPath)
    {
        outHash = g_pSystem->Replay.CalculateFileHash(path);
        g_pState->Replay.SetPreviousReplayPath(path);
    }
}

void ReplayManagerTab::DrawSessionPath(const std::string& path)
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "File:");
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    ImGui::InputText("##CurrentPath", (char*)path.c_str(), path.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Right-click to copy path to clipboard.");

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            ImGui::SetClipboardText(path.c_str());
        }
    }
}

void ReplayManagerTab::DrawSessionActions(const std::string& path, const std::string& hash)
{
    if (ImGui::Button("Save Replay to Library"))
    {
        g_pSystem->Replay.SaveReplay(path);
        g_pState->Replay.SetRefreshReplayList(true);
    }

    ImGui::SameLine();

    std::filesystem::path replayFolder = std::filesystem::path(g_pState->Settings.GetAppDataDirectory()) / "Replays" / hash;
    bool replayExists = std::filesystem::exists(replayFolder);

    if (!replayExists) ImGui::BeginDisabled();
    if (ImGui::Button("Backup Timeline")) g_pSystem->Replay.SaveTimeline(hash);
    if (!replayExists)
    {
        ImGui::EndDisabled();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("You must save the Replay to the library first.");
        }
    }

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.1f, 0.1f, 1.0f));

    if (ImGui::Button("Clear Session")) g_pState->Replay.ClearActiveSession();

    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Discards the current session.");
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
            g_pSystem->Replay.DeleteReplay(m_HashToDelete);
            g_pState->Replay.SetRefreshReplayList(true);
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
            g_pSystem->Replay.DeleteInGameReplay(m_PathToDelete);
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