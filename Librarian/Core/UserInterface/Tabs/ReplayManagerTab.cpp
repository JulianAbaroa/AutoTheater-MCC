#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/GlobalState.h"
#include "Core/Common/PersistenceManager.h"
#include "Core/UserInterface/Tabs/ReplayManagerTab.h"
#include <filesystem>

void ReplayManagerTab::Draw()
{
    ImGui::TextDisabled("Current Replay Detected:");
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

        ImGui::BulletText("Path: %s", currentFilmPath.c_str());

        if (ImGui::Button("Save replay (.mov)")) 
        {
            std::string path;
            { 
                std::lock_guard lock(g_pState->replayManagerMutex); 
                path = g_pState->filmPath; 
            }

            PersistenceManager::SaveReplay(path);
            g_pState->refreshReplayList.store(true);
        }

        ImGui::SameLine();
        std::filesystem::path replayFolder = std::filesystem::path(g_pState->appDataDirectory) / "Replays" / cachedHash;
        bool replayExists = std::filesystem::exists(replayFolder);

        if (!replayExists) ImGui::BeginDisabled();

        if (ImGui::Button("Save Timeline")) 
        { 
            PersistenceManager::SaveTimeline(cachedHash); 
        }

        if (!replayExists) 
        {
            ImGui::EndDisabled();

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::SetTooltip("You must save the Replay (.mov) first.");
            }
        }
    }
    else 
    {
        ImGui::TextWrapped("No replay has been detected. Open one in the game to begin.");
    }

    ImGui::Spacing(); ImGui::Spacing();
    ImGui::Text("Library (Saved Replays)");
    ImGui::Separator();

    static char searchBuffer[128] = "";
    ImGui::InputText("Search", searchBuffer, IM_ARRAYSIZE(searchBuffer));
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

    if (ImGui::BeginChild("ReplayList", ImVec2(0, 400), true))
    {
        if (savedReplaysCopy.empty()) 
        {
            ImGui::TextDisabled("Library is empty...");
        }
        else 
        {
            for (int i = 0; i < (int)savedReplaysCopy.size(); i++)
            {
                auto& replay = savedReplaysCopy[i];
                if (strlen(searchBuffer) > 0) 
                {
                    std::string s = searchBuffer; std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                    std::string name = replay.DisplayName; std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    if (name.find(s) == std::string::npos) continue;
                }

                ImGui::PushID(i);

                ImVec2 itemPos = ImGui::GetCursorScreenPos();
                float width = ImGui::GetContentRegionAvail().x;
                float height = 150.0f;
                float btnWidth = 200.0f;
                float padding = 10.0f; 

                ImU32 bgColor = (selectedIndex == i) ? ImGui::GetColorU32(ImVec4(0.12f, 0.14f, 0.18f, 1.0f)) : ImGui::GetColorU32(ImGuiCol_WindowBg);

                ImGui::GetWindowDrawList()->AddRectFilled(itemPos, ImVec2(itemPos.x + width, itemPos.y + height), bgColor, 6.0f);
                ImGui::GetWindowDrawList()->AddRect(itemPos, ImVec2(itemPos.x + width, itemPos.y + height), ImGui::GetColorU32(ImGuiCol_Border), 6.0f);

                ImGui::SetCursorScreenPos(itemPos);
                if (ImGui::InvisibleButton("##hitbox", ImVec2(width - btnWidth, height))) 
                {
                    selectedIndex = i;
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) 
                {
                    editingIndex = i;
                    strncpy_s(renameBuf, replay.DisplayName.c_str(), sizeof(renameBuf));
                }

                ImGui::SetCursorScreenPos(ImVec2(itemPos.x + 15, itemPos.y + 15));
                ImGui::BeginGroup();
                if (editingIndex == i) 
                {
                    ImGui::SetNextItemWidth(250);
                    ImGui::SetKeyboardFocusHere(); 

                    if (ImGui::InputText("##rename_input", renameBuf, IM_ARRAYSIZE(renameBuf), ImGuiInputTextFlags_EnterReturnsTrue)) 
                    {
                        PersistenceManager::RenameReplay(replay.Hash, renameBuf);
                        g_pState->refreshReplayList.store(true);
                        editingIndex = -1;
                    }

                    if (ImGui::IsItemDeactivated() && !ImGui::IsItemHovered()) editingIndex = -1;
                }
                else 
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", replay.DisplayName.c_str());
                }

                ImGui::TextDisabled("Author: %s", replay.Author.c_str());

                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + (width - btnWidth - 40));
                if (!replay.Info.empty() && replay.Info != "N/A")
                {
                    ImGui::TextDisabled("Info: %s", replay.Info.c_str());
                }
                else
                {
                    ImGui::Dummy(ImVec2(0, 15));
                }

                ImGui::PopTextWrapPos();
                ImGui::EndGroup();

                ImGui::SetCursorScreenPos(ImVec2(itemPos.x + 15, itemPos.y + height - 25));
                if (replay.HasTimeline)
                {
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "Timeline Available");
                }
                else
                {
                    ImGui::TextDisabled("No timeline found");
                }

                ImGui::SetCursorScreenPos(ImVec2(itemPos.x + width - btnWidth - 10, itemPos.y + 12));
                ImGui::BeginGroup();
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

                if (ImGui::Button("Restore to Game", ImVec2(btnWidth, 32))) 
                {
                    PersistenceManager::RestoreReplay(replay);
                }

                ImGui::Spacing();

                if (replay.HasTimeline) 
                {
                    if (ImGui::Button("Load Timeline", ImVec2(btnWidth, 32)))
                    {
                        PersistenceManager::LoadTimeline(replay.Hash);
                    }
                }
                else 
                {
                    ImGui::BeginDisabled();
                    ImGui::Button("No Timeline", ImVec2(btnWidth, 32));
                    ImGui::EndDisabled();
                }

                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));

                if (ImGui::Button("Delete from Library", ImVec2(btnWidth, 32))) 
                {
                    hashToDelete = replay.Hash;
                    openDeleteModal = true;
                }

                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
                ImGui::EndGroup();

                ImGui::PopID();

                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padding);
                ImGui::Dummy(ImVec2(0.0f, 0.0f));
            }
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
        ImGui::Text("This replay will be permanently removed from your library.\nAre you sure?");
        ImGui::Separator();

        if (ImGui::Button("Yes, Delete", ImVec2(150, 0))) 
        {
            PersistenceManager::DeleteReplay(hashToDelete);
            g_pState->refreshReplayList.store(true);
            selectedIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(150, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}