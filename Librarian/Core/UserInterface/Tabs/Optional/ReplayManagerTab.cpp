#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/AppCore.h"
#include "Core/Common/PersistenceManager.h"
#include "Core/UserInterface/Tabs/Optional/ReplayManagerTab.h"
#include "External/imgui/imgui.h" 
#include <filesystem>
#include <algorithm>

void ReplayManagerTab::Draw()
{
    DrawCurrentSession();

    ImGui::Spacing(); 
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

    if (OpenLibDeleteModal)
    {
        ImGui::OpenPopup("Delete Library Replay");
        OpenLibDeleteModal = false;
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
            g_pSystem->Replay.DeleteReplay(HashToDelete);
            g_pState->Replay.SetRefreshReplayList(true); 
            SelectedLibIndex = -1; 
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (OpenGameDeleteModal)
    {
        ImGui::OpenPopup("Delete Game Replay");
        OpenGameDeleteModal = false;
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
            g_pSystem->Replay.DeleteInGameReplay(PathToDelete);
            NeedsInGameRefresh = true;
            if (SelectedGameIndex >= 0) SelectedGameIndex = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void ReplayManagerTab::DrawLibrary()
{
    DrawSearchBar("##search_inlibrary", LibrarySearchBuffer, IM_ARRAYSIZE(LibrarySearchBuffer));
    ImGui::Spacing();

    if (g_pState->Replay.ShouldRefreshReplayList())
    {
        auto data = g_pSystem->Replay.GetSavedReplays();
        g_pState->Replay.SetSavedReplaysCache(data);
        g_pState->Replay.SetRefreshReplayList(false);
    }

    std::vector<SavedReplay> savedReplaysCopy = g_pState->Replay.GetSavedReplaysCacheCopy();

    std::string searchStr = LibrarySearchBuffer;
    bool hasFilter = !searchStr.empty();
    if (hasFilter)
    {
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);
    }

    int visibleCount = 0;
    for (const auto& r : savedReplaysCopy) {
        if (!hasFilter) 
        { 
            visibleCount = (int)savedReplaysCopy.size(); 
            break; 
        }

        std::string n = r.DisplayName; std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        std::string a = r.TheaterReplay.FilmMetadata.Author; std::transform(a.begin(), a.end(), a.begin(), ::tolower);

        if (n.find(searchStr) != std::string::npos ||
            a.find(searchStr) != std::string::npos)
        {
            visibleCount++;
        }
    }

    ImGui::AlignTextToFramePadding();

    if (savedReplaysCopy.empty())
    {
        ImGui::TextDisabled("Library is empty...");
    }
    else
    {
        ImGui::TextDisabled("Showing %d saved replays", visibleCount);
    }

    float buttonWidth = ImGui::CalcTextSize("Refresh List").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - buttonWidth);

    if (ImGui::Button("Refresh List"))
    {
        g_pState->Replay.SetRefreshReplayList(true);
        SelectedLibIndex = -1;
    }

    ImGui::Separator();

    if (ImGui::BeginChild("ReplayList", ImVec2(0, 0), ImGuiChildFlags_Borders))
    {
        if (!savedReplaysCopy.empty())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12.0f, 8.0f));

            for (int i = 0; i < (int)savedReplaysCopy.size(); i++)
            {
                auto& replay = savedReplaysCopy[i];

                if (hasFilter)
                {
                    std::string name = replay.DisplayName;
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

                    std::string author = replay.TheaterReplay.FilmMetadata.Author;
                    std::transform(author.begin(), author.end(), author.begin(), ::tolower);

                    std::string info = replay.TheaterReplay.FilmMetadata.Info;
                    std::transform(info.begin(), info.end(), info.begin(), ::tolower);

                    if (name.find(searchStr) == std::string::npos &&
                        author.find(searchStr) == std::string::npos &&
                        info.find(searchStr) == std::string::npos) 
                    {
                        continue;
                    }
                }

                ImGui::PushID(replay.Hash.c_str());
                bool isSelected = (SelectedLibIndex == i);

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
                    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
                    {
                        SelectedLibIndex = i;
                    }

                    if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(0)) 
                    {
                        EditingIndex = i;
                        strncpy_s(RenameBuf, replay.DisplayName.c_str(), sizeof(RenameBuf));
                    }

                    if (ImGui::BeginTable("ReplayItemTable", 2, ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch, 0.65f);
                        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch, 0.35f);
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);

                        if (EditingIndex == i)
                        {
                            ImGui::SetNextItemWidth(-1);
                            ImGui::SetKeyboardFocusHere();
                            if (ImGui::InputText("##rename_input", RenameBuf, IM_ARRAYSIZE(RenameBuf), ImGuiInputTextFlags_EnterReturnsTrue))

                            {
                                g_pSystem->Replay.RenameReplay(replay.Hash, RenameBuf);
                                g_pState->Replay.SetRefreshReplayList(true);
                                EditingIndex = -1;
                            }

                            if (ImGui::IsItemDeactivated() && !ImGui::IsItemActive())
                            {
                                EditingIndex = -1;
                            }
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", replay.DisplayName.c_str());
                        }

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
                            HashToDelete = replay.Hash;
                            OpenLibDeleteModal = true;
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

            ImGui::PopStyleVar();
        }
    }

    ImGui::EndChild();
}

void ReplayManagerTab::DrawInGameReplays()
{
    DrawSearchBar("##search_ingame", InGameSearchBuffer, IM_ARRAYSIZE(InGameSearchBuffer));
    ImGui::Spacing();

    if (NeedsInGameRefresh)
    {
        std::filesystem::path tempDir = g_pState->Settings.GetMovieTempDirectory();
        CachedInGameReplays = g_pSystem->Replay.GetTheaterReplays(tempDir);

        std::erase_if(CachedInGameReplays, [](const TheaterReplay& r) {
            return r.MovFileName == ".hotreload_trigger.mov";
        });

        std::sort(CachedInGameReplays.begin(), CachedInGameReplays.end(), [](const TheaterReplay& a, const TheaterReplay& b) {
            return std::filesystem::last_write_time(a.FullPath) > std::filesystem::last_write_time(b.FullPath);
        });

        NeedsInGameRefresh = false;
    }

    std::string searchStr = InGameSearchBuffer;
    bool hasFilter = !searchStr.empty();
    if (hasFilter)
    {
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);
    }

    int visibleCount = 0;
    if (hasFilter)
    {
        for (const auto& r : CachedInGameReplays)
        {
            std::string n = r.MovFileName; std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            std::string a = r.FilmMetadata.Author; std::transform(a.begin(), a.end(), a.begin(), ::tolower);
            std::string info = r.FilmMetadata.Info; std::transform(info.begin(), info.end(), info.begin(), ::tolower);

            if (n.find(searchStr) != std::string::npos ||
                a.find(searchStr) != std::string::npos ||
                info.find(searchStr) != std::string::npos)
            {
                visibleCount++;
            }
        }
    }
    else
    {
        visibleCount = (int)CachedInGameReplays.size();
    }

    ImGui::AlignTextToFramePadding();
    if (CachedInGameReplays.empty())
    {
        ImGui::TextDisabled("No active replays in game directory.");
    }
    else
    {
        ImGui::TextDisabled("Showing %d active replays", visibleCount);
    }

    float buttonWidth = ImGui::CalcTextSize("Refresh List").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - buttonWidth);
    if (ImGui::Button("Refresh List"))
    {
        NeedsInGameRefresh = true;
        SelectedGameIndex = -1;
    }

    ImGui::Separator();

    if (CachedInGameReplays.empty()) return;

    if (ImGui::BeginChild("InGameList", ImVec2(0, 0), ImGuiChildFlags_Borders))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12.0f, 8.0f));

        for (int i = 0; i < (int)CachedInGameReplays.size(); i++)
        {
            auto& replay = CachedInGameReplays[i];

            if (hasFilter)
            {
                std::string n = replay.MovFileName; std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                std::string a = replay.FilmMetadata.Author; std::transform(a.begin(), a.end(), a.begin(), ::tolower);
                std::string info = replay.FilmMetadata.Info; std::transform(info.begin(), info.end(), info.begin(), ::tolower);

                if (n.find(searchStr) == std::string::npos &&
                    a.find(searchStr) == std::string::npos &&
                    info.find(searchStr) == std::string::npos)
                {
                    continue;
                }
            }

            bool isSelected = (SelectedGameIndex == i);
            ImGui::PushID(i);

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
                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
                {
                    SelectedGameIndex = i;
                }

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
                        PathToDelete = replay.FullPath;
                        OpenGameDeleteModal = true;
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

        ImGui::PopStyleVar();
    }

    ImGui::EndChild();
}


void ReplayManagerTab::DrawCurrentSession()
{
    ImGui::TextDisabled("ACTIVE SESSION");
    ImGui::Separator();

    std::string currentFilmPath = g_pState->Replay.GetCurrentFilmPath();
    std::string lastProcessedPath = g_pState->Replay.GetPreviousReplayPath();

    if (!currentFilmPath.empty())
    {
        static std::string cachedHash = "";

        if (currentFilmPath != lastProcessedPath)
        {
            cachedHash = g_pSystem->Replay.CalculateFileHash(currentFilmPath);
            g_pState->Replay.SetPreviousReplayPath(g_pState->Replay.GetCurrentFilmPath());
        }

        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "File:");
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##CurrentPath", (char*)currentFilmPath.c_str(), currentFilmPath.size(), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Right-click to copy path to clipboard.");
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                ImGui::SetClipboardText(currentFilmPath.c_str());
            }
        }

        ImGui::Spacing();

        if (ImGui::Button("Save Replay to Library"))
        {
            g_pSystem->Replay.SaveReplay(currentFilmPath);
            g_pState->Replay.SetRefreshReplayList(true);
        }

        ImGui::SameLine();
        std::filesystem::path replayFolder =
            std::filesystem::path(g_pState->Settings.GetAppDataDirectory()) / "Replays" / cachedHash;
        bool replayExists = std::filesystem::exists(replayFolder);

        if (!replayExists) ImGui::BeginDisabled();

        if (ImGui::Button("Backup Timeline"))
        {
            g_pSystem->Replay.SaveTimeline(cachedHash);
        }

        if (!replayExists)
        {
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("You must save the Replay to the library first.");
        }
    }
    else
    {
        ImGui::TextDisabled("No active theater film detected. Open a replay in Halo MCC.");
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
    if (ImGui::InputTextWithHint(label, "Search by name, map or author...", buffer, bufferSize)) {}
    ImGui::PopItemWidth();
    ImGui::SameLine();

    if (ImGui::Button("Clear", ImVec2(clearBtnWidth, 0)))
    {
        buffer[0] = '\0';
    }
}