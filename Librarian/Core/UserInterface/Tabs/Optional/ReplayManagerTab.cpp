#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/GlobalState.h"
#include "Core/Common/PersistenceManager.h"
#include "Core/UserInterface/Tabs/Optional/ReplayManagerTab.h"
#include "External/imgui/imgui.h" 
#include <filesystem>
#include <algorithm>

static void DrawCurrentSession()
{
    ImGui::TextDisabled("ACTIVE SESSION");
    ImGui::Separator();

    std::string currentFilmPath;
    std::string lastProcessedPath;
    {
        std::lock_guard lock(g_pState->replayManagerMutex);
        currentFilmPath = g_pState->filmPath;
        lastProcessedPath = g_pState->lastProcessedPath;
    }

    if (!currentFilmPath.empty())
    {
        static std::string cachedHash = "";

        if (currentFilmPath != lastProcessedPath)
        {
            cachedHash = PersistenceManager::CalculateFileHash(currentFilmPath);
            {
                std::lock_guard lock(g_pState->replayManagerMutex);
                g_pState->lastProcessedPath = g_pState->filmPath;
            }
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
            PersistenceManager::SaveReplay(currentFilmPath);
            g_pState->refreshReplayList.store(true);
        }

        ImGui::SameLine();
        std::filesystem::path replayFolder = std::filesystem::path(g_pState->appDataDirectory) / "Replays" / cachedHash;
        bool replayExists = std::filesystem::exists(replayFolder);

        if (!replayExists) ImGui::BeginDisabled();

        if (ImGui::Button("Backup Timeline"))
        {
            PersistenceManager::SaveTimeline(cachedHash);
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

static void DrawSearchBar(char* buffer, size_t bufferSize)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Filter:");
    ImGui::SameLine();

    float clearBtnWidth = 60.0f;
    float availableWidth = ImGui::GetContentRegionAvail().x;

    ImGui::PushItemWidth(availableWidth - clearBtnWidth - ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::InputTextWithHint("##SearchReplay", "Search by name, map or author...", buffer, bufferSize)) { }
    ImGui::PopItemWidth();
    ImGui::SameLine();

    if (ImGui::Button("Clear", ImVec2(clearBtnWidth, 0)))
    {
        buffer[0] = '\0';
    }
}

void ReplayManagerTab::Draw()
{
    DrawCurrentSession();

    ImGui::Spacing(); ImGui::Spacing();
    ImGui::TextDisabled("REPLAY LIBRARY");
    ImGui::Separator();

    static char searchBuffer[128] = "";
    DrawSearchBar(searchBuffer, IM_ARRAYSIZE(searchBuffer));

    ImGui::Spacing();

    static int selectedIndex = -1;
    static int editingIndex = -1;
    static char renameBuf[128] = "";
    static std::string hashToDelete = "";
    static bool openDeleteModal = false;

    if (g_pState->refreshReplayList.load())
    {
        std::lock_guard lock(g_pState->replayManagerMutex);
        g_pState->savedReplaysCache = PersistenceManager::GetSavedReplays();
        g_pState->refreshReplayList.store(false);
    }

    std::vector<SavedReplay> savedReplaysCopy;
    {
        std::lock_guard lock(g_pState->replayManagerMutex);
        savedReplaysCopy = g_pState->savedReplaysCache;
    }

    if (ImGui::BeginChild("ReplayList", ImVec2(0, 0), ImGuiChildFlags_Borders))
    {
        if (savedReplaysCopy.empty())
        {
            ImGui::TextDisabled("Library is empty...");
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12.0f, 8.0f));

            for (int i = 0; i < (int)savedReplaysCopy.size(); i++)
            {
                auto& replay = savedReplaysCopy[i];

                if (strlen(searchBuffer) > 0)
                {
                    std::string s = searchBuffer;
                    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                    std::string name = replay.DisplayName;
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    std::string info = replay.Info;
                    std::transform(info.begin(), info.end(), info.begin(), ::tolower);
                    if (name.find(s) == std::string::npos && info.find(s) == std::string::npos)
                        continue;
                }

                ImGui::PushID(i);

                bool isSelected = (selectedIndex == i);

                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

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

                if (ImGui::BeginChild(("ReplayItem_" + std::to_string(i)).c_str(), ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY))
                {
                    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
                    {
                        selectedIndex = i;
                    }

                    if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(0))
                    {
                        editingIndex = i;
                        strncpy_s(renameBuf, replay.DisplayName.c_str(), sizeof(renameBuf));
                    }

                    if (ImGui::BeginTable("ReplayItemTable", 2, ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch, 0.65f);
                        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch, 0.35f);

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);

                        if (editingIndex == i)
                        {
                            ImGui::SetNextItemWidth(-1);
                            ImGui::SetKeyboardFocusHere();
                            if (ImGui::InputText("##rename_input", renameBuf, IM_ARRAYSIZE(renameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
                            {
                                PersistenceManager::RenameReplay(replay.Hash, renameBuf);
                                g_pState->refreshReplayList.store(true);
                                editingIndex = -1;
                            }
                            if (ImGui::IsItemDeactivated() && !ImGui::IsItemActive())
                                editingIndex = -1;
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", replay.DisplayName.c_str());
                        }

                        ImGui::TextDisabled("Author: %s", replay.Author.c_str());

                        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                        ImGui::TextWrapped("%s", replay.Info.empty() ? "No info available" : replay.Info.c_str());
                        ImGui::PopStyleColor();

                        ImGui::Spacing();

                        if (replay.HasTimeline)
                            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "[Timeline Available]");
                        else
                            ImGui::TextDisabled("[No Timeline]");

                        ImGui::TableSetColumnIndex(1);

                        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

                        if (ImGui::Button("Restore to Game", ImVec2(-FLT_MIN, 0)))
                        {
                            PersistenceManager::RestoreReplay(replay);
                        }

                        if (!replay.HasTimeline) ImGui::BeginDisabled();
                        if (ImGui::Button("Load Timeline", ImVec2(-FLT_MIN, 0)))
                        {
                            PersistenceManager::LoadTimeline(replay.Hash);
                        }
                        if (!replay.HasTimeline) ImGui::EndDisabled();

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));

                        if (ImGui::Button("Delete", ImVec2(-FLT_MIN, 0)))
                        {
                            hashToDelete = replay.Hash;
                            openDeleteModal = true;
                        }

                        ImGui::PopStyleColor(3);
                        ImGui::PopStyleVar();

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

    if (openDeleteModal)
    {
        ImGui::OpenPopup("DeleteConfirmPopup");
        openDeleteModal = false;
    }

    if (ImGui::BeginPopupModal("DeleteConfirmPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("This replay will be permanently removed from your library.\nThis action cannot be undone.");
        ImGui::Separator();
        ImGui::Spacing();

        float btnW = 120.0f;
        float totalW = btnW * 2 + ImGui::GetStyle().ItemSpacing.x;
        float offset = (ImGui::GetContentRegionAvail().x - totalW) * 0.5f;
        if (offset > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

        if (ImGui::Button("Yes, Delete", ImVec2(btnW, 0)))
        {
            PersistenceManager::DeleteReplay(hashToDelete);
            g_pState->refreshReplayList.store(true);
            selectedIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(btnW, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}