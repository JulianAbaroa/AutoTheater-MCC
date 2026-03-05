#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/UI/Tabs/Optional/CaptureTab.h"
#include <shobjidl.h>

// TODO: Add the recording time.

void CaptureTab::Draw()
{
    bool isRecording = g_pState->FFmpeg.IsRecording();

	this->DrawTopBar(isRecording);

	ImGui::Separator();
	ImGui::Spacing();

	if (g_pState->FFmpeg.IsFFmpegInstalled())
	{
        this->DrawGallery(isRecording);
	}

    this->DrawRecordingSettingsPopup();
    this->DrawPopups();
}

void CaptureTab::DrawTopBar(bool isRecording)
{
    bool isCaptureActive = g_pState->FFmpeg.IsCaptureActive();
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float midPoint = totalWidth / 2.0f;

    if (g_pState->FFmpeg.IsFFmpegInstalled())
    {
        // Left side of the TopBar.
        this->DrawRecordingControls(isRecording, isCaptureActive);
    }
    else
    {
        ImGui::BeginGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Recording Controls Disabled");
        ImGui::EndGroup();
    }

    ImGui::SameLine(midPoint);
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Right side of the TopBar.
    this->DrawFFmpegControls(isRecording, totalWidth);
}

void CaptureTab::DrawRecordingControls(bool isRecording, bool isCaptureActive)
{
    ImGui::BeginGroup();

    const char* statusText = "WAITING";
    ImVec4 statusColor = ImVec4(1, 1, 1, 1);

    if (g_pState->FFmpeg.RecordingStarted() || (isRecording && !isCaptureActive))
    {
        statusText = "STARTING";
        statusColor = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
    }
    else if (isCaptureActive)
    {
        statusText = "RECORDING";
        statusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    else if (g_pState->FFmpeg.RecordingStopped())
    {
        statusText = "STOPPING";
        statusColor = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Status:");

    ImGui::SameLine();

    ImGui::TextColored(statusColor, statusText);

    ImGui::SameLine(0, 10.0f);
    if (!isRecording)
    {
        bool canRecord = g_pState->Theater.IsTheaterMode() &&
            g_pState->Theater.GetTimePtr() != nullptr &&
            g_pState->Director.IsInitialized() &&
            g_pState->Audio.GetMasterInstance() != nullptr;

        if (!canRecord) ImGui::BeginDisabled();

        if (ImGui::Button("Start Recording"))
        {
            g_pState->FFmpeg.SetStartRecording(true);
        }

        if (!canRecord)
        {
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::BeginTooltip();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Recording unavailable:");

                if (!g_pState->Theater.IsTheaterMode())             ImGui::BulletText("Theater Mode is not active");
                if (g_pState->Theater.GetTimePtr() == nullptr)      ImGui::BulletText("Theater Time not found");
                if (!g_pState->Director.IsInitialized())            ImGui::BulletText("Director is not initialized");
                if (g_pState->Audio.GetMasterInstance() == nullptr) ImGui::BulletText("Audio Master Instance missing");

                ImGui::EndTooltip();
            }

            ImGui::EndDisabled();
        }
    }
    else
    {
        if (!isCaptureActive) ImGui::BeginDisabled();

        const char* btnLabel = !isCaptureActive ? "  ...  " : "Stop Recording";

        if (ImGui::Button(btnLabel))
        {
            g_pState->FFmpeg.SetStopRecording(true);
            g_pSystem->Gallery.RefreshList(g_pState->FFmpeg.GetOutputPath());
        }

        if (!isCaptureActive)
        {
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::SetTooltip("Please wait while AutoTheater initializes FFmpeg and related systems...");
            }

            ImGui::EndDisabled();
        }
    }

    ImGui::SameLine(0, 10.0f);

    if (isRecording) ImGui::BeginDisabled();
    if (ImGui::Button("Recording Settings"))
    {
        m_OpenRecordingSettingsModal.store(true);
    }
    if (isRecording) ImGui::EndDisabled();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && isRecording)
    {
        ImGui::SetTooltip("These settings cannot be changed while AutoTheater is recording.");
    }

    ImGui::EndGroup();
}

void CaptureTab::DrawFFmpegControls(bool isRecoring, float totalWidth)
{
    ImGui::BeginGroup();

    float padding = ImGui::GetStyle().ItemSpacing.x;

    if (g_pState->FFmpeg.IsDownloadInProgress())
    {
        float barW = 180.0f;
        float btnW = ImGui::CalcTextSize("Cancel").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float groupW = barW + padding + btnW;

        ImGui::SetCursorPosX(totalWidth - groupW - padding);

        float progress = g_pState->FFmpeg.GetDownloadProgress() / 100.0f;

        char buf[32]{};
        sprintf_s(buf, "%.0f%%", progress * 100.0f);

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));

        ImGui::SetNextItemWidth(barW);
        ImGui::ProgressBar(progress, ImVec2(0, 0), buf);

        ImGui::PopStyleColor();
        ImGui::SameLine();

        bool isFinished = (progress >= 1.0f);
        if (isFinished) ImGui::BeginDisabled();

        if (ImGui::Button("Cancel"))
        {
            g_pSystem->FFmpeg.CancelDownload();
        }

        if (isFinished) ImGui::EndDisabled();
    }
    else if (g_pState->FFmpeg.IsFFmpegInstalled())
    {
        float textW = ImGui::CalcTextSize("FFmpeg Installed").x;
        float btnW = ImGui::CalcTextSize("Uninstall").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float groupW = textW + padding + btnW;

        ImGui::SetCursorPosX(totalWidth - groupW - padding);

        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "FFmpeg Installed");
        ImGui::SameLine();

        if (isRecoring) ImGui::BeginDisabled();

        if (ImGui::Button("Uninstall"))
        {
            m_OpenUninstallModal.store(true);
        }

        if (isRecoring)
        {
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::BeginTooltip();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Action Blocked:");
                ImGui::BulletText("Cannot uninstall FFmpeg while recording is in progress.");
                ImGui::BulletText("Stop the current session before removing dependencies.");
                ImGui::EndTooltip();
            }

            ImGui::EndDisabled();
        }
    }
    else
    {
        float textW = ImGui::CalcTextSize("FFmpeg not found").x;
        float btnW = ImGui::CalcTextSize("Download FFmpeg").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float groupW = textW + padding + btnW;

        ImGui::SetCursorPosX(totalWidth - groupW - padding);

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("FFmpeg not found");
        ImGui::SameLine();

        if (ImGui::Button("Download FFmpeg"))
        {
            g_pSystem->FFmpeg.DownloadFFmpeg();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("FFmpeg is required for recording and processing video.");
        }
    }
    ImGui::EndGroup();
}


void CaptureTab::DrawRecordingSettingsPopup()
{
    if (m_OpenRecordingSettingsModal.load())
    {
        ImGui::OpenPopup("Recording Settings");
        m_OpenRecordingSettingsModal.store(false);
    }

    bool keepOpen = true;
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0, 0, 0, 0));
    if (ImGui::BeginPopupModal("Recording Settings", &keepOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
        {
            ImGui::CloseCurrentPopup();
        }
        this->DrawRecordingSettings();

        if (!keepOpen) ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor();
}

void CaptureTab::DrawRecordingSettings()
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (!m_PendingNewPath.empty())
        {
            g_pState->FFmpeg.SetOutputPath(m_PendingNewPath);
            g_pSystem->Gallery.RefreshList(m_PendingNewPath);
            m_PendingNewPath = "";
        }
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Resolution Scale:");
    ImGui::SameLine(160);

    const char* resolutions[] = { "1080p", "1440p", "2160p" };
    int currentRes = (int)g_pState->FFmpeg.GetResolutionType();

    ImGui::SetNextItemWidth(150);
    if (ImGui::Combo("##ResCombo", &currentRes, resolutions, IM_ARRAYSIZE(resolutions)))
    {
        g_pState->FFmpeg.SetResolutionType((ResolutionType)currentRes);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::BeginTooltip();

        ImGui::Text("Output Resolution Scaling");
        ImGui::Spacing();
        ImGui::TextDisabled("Available scales (1080p, 1440p, 2160p) are software-level\n"
            "adjustments applied via FFmpeg's bicubic filter.");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Smart Scaling");
        ImGui::Text("If the selected scale matches your current in-game resolution,\n"
            "no scaling is applied. The frames are passed directly to\n"
            "the encoder to preserve 1:1 original pixel quality.");

        ImGui::EndTooltip();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Target Framerate:");
    ImGui::SameLine(160);

    int currentFPS = (int)g_pState->FFmpeg.GetTargetFramerate();

    ImGui::SetNextItemWidth(150);
    if (ImGui::SliderInt("##FPSSlider", &currentFPS, 60, 240, "%d FPS"))
    {
        if (currentFPS > 240) currentFPS = 240;
        else if (currentFPS < 60) currentFPS = 60;
        g_pState->FFmpeg.SetTargetFramerate((float)currentFPS);
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Adjust the recording frame rate.");
        ImGui::TextDisabled("Allowed range: 60 - 240 FPS.");
        ImGui::EndTooltip();
    }

    bool recordUI = g_pState->FFmpeg.ShouldRecordUI();
    bool stopOnLast = g_pState->FFmpeg.StopOnLastEvent();

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Interface:");
    ImGui::SameLine(160);

    if (ImGui::Checkbox("Capture ImGui Overlay", &recordUI))
    {
        g_pState->FFmpeg.SetRecordUI(recordUI);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("If enabled, the video will include the AutoTheater UI.");
    }

    ImGui::SetCursorPosX(160);

    if(ImGui::Checkbox("Stop on last script event", &stopOnLast))
    {
        g_pState->FFmpeg.SetStopOnLastEvent(stopOnLast);
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("The recording will automatically finish once the Director\n"
            "executes the last command in the current script.");
    }

    float stopDelay = g_pState->FFmpeg.GetStopDelayDuration();

    if (!stopOnLast) ImGui::BeginDisabled();

    ImGui::SetCursorPosX(175);
    ImGui::SetNextItemWidth(135);

    if (ImGui::SliderFloat("Post-roll Delay##DelaySlider", &stopDelay, 0.0f, 20.0f, "%.1fs"))
    {
        if (stopDelay > 20.0f) stopDelay = 20.0f;
        else if (stopDelay < 0.0f) stopDelay = 0.0f;

        g_pState->FFmpeg.SetStopDelayDuration(stopDelay);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Additional seconds to record after the last event executes.");
    }

    if (!stopOnLast) ImGui::EndDisabled();

    ImGui::Spacing();

    ImGui::Text("Output Directory:");

    std::string outputPath = g_pState->FFmpeg.GetOutputPath();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
    ImGui::SetNextItemWidth(300.0f);
    ImGui::InputText("##OutputPath", (char*)outputPath.c_str(), outputPath.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine();

    if (m_FolderPickerActive.load()) ImGui::BeginDisabled();

    const char* btnText = m_FolderPickerActive.load() ? "Opening..." : "Select Folder";
    if (ImGui::Button(btnText))
    {
        m_FolderPickerActive.store(true);

        std::thread([this]() {
            std::string selectedPath = this->OpenFolderDialog();

            if (!selectedPath.empty())
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                this->m_PendingNewPath = selectedPath;
            }

            this->m_FolderPickerActive.store(false);
        }).detach();
    }

    if (m_FolderPickerActive.load()) ImGui::EndDisabled();

    ImGui::EndPopup();
}

void CaptureTab::DrawGallery(bool isRecording)
{
    const std::vector<VideoData> videos = g_pState->Gallery.GetVideos();
    int videoCount = (int)videos.size();

    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Recorded Videos (%d)", videoCount);

    size_t pending = g_pSystem->Gallery.GetPendingCount();
    if (pending > 0) 
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "  (Processing: %zu left)", pending);
    }

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 130);
    if (isRecording) ImGui::BeginDisabled();
    if (ImGui::Button("Refresh Gallery", ImVec2(130, 0))) 
    {
        g_pSystem->Gallery.RefreshList(g_pState->FFmpeg.GetOutputPath());
    }
    if (isRecording) ImGui::EndDisabled();

    if (isRecording && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Cannot refresh the gallery while recording.");
    }

    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginChild("VideoGalleryScrollArea", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None))
    {
        if (videos.empty())
        {
            ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.4f);
            const char* emptyTxt = "No videos found in output directory.";
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(emptyTxt).x) * 0.5f);
            ImGui::TextDisabled(emptyTxt);
        }
        else 
        {
            float cardWidth = 280.0f;
            float availWidth = ImGui::GetContentRegionAvail().x;
            int columns = (std::max)(1, (int)(availWidth / (cardWidth + ImGui::GetStyle().ItemSpacing.x)));
            if (columns > 4) columns = 4;

            if (ImGui::BeginTable("VideoGalleryTable", columns, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings))
            {
                for (int n = 0; n < columns; n++)
                    ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, cardWidth);

                for (int i = 0; i < videoCount; i++)
                {
                    const auto& video = videos[i];

                    if (!video.IsMetadataLoaded && !video.IsLoading)
                    {
                        g_pState->Gallery.SetLoading(i, true);
                        g_pSystem->Gallery.LoadMetadataAsync(i);
                    }

                    ImGui::TableNextColumn();
                    ImGui::PushID(i);

                    float cellWidth = ImGui::GetContentRegionAvail().x;
                    bool isSelected = (g_pState->Gallery.GetSelectedIndex() == i);

                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, isSelected ? ImVec4(0.18f, 0.22f, 0.30f, 1.0f) : ImVec4(0.12f, 0.12f, 0.12f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_Border, isSelected ? ImVec4(0.40f, 0.60f, 1.0f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

                    if (ImGui::BeginChild("VideoCard", ImVec2(cellWidth, 320), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
                    {
                        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
                            g_pState->Gallery.SetSelectedIndex(i);

                        const float pad = 12.0f;
                        const float contentWidth = cellWidth - (pad * 2.0f);

                        float thumbHeight = contentWidth * (9.0f / 16.0f);
                        ImGui::SetCursorPos({ pad, pad });
                        ImVec2 curPos = ImGui::GetCursorScreenPos();
                        ImGui::GetWindowDrawList()->AddRectFilled(curPos, { curPos.x + contentWidth, curPos.y + thumbHeight }, IM_COL32(25, 25, 25, 255), 4.0f);

                        if (video.Thumbnail) 
                        {
                            ImGui::SetCursorPos({ pad, pad });
                            ImGui::Image((ImTextureID)video.Thumbnail, ImVec2(contentWidth, thumbHeight));
                        }
                        else 
                        {
                            const char* txt = video.IsMetadataLoaded ? "NO THUMBNAIL" : "LOADING...";
                            ImVec2 txtSize = ImGui::CalcTextSize(txt);
                            ImGui::GetWindowDrawList()->AddText({ curPos.x + (contentWidth - txtSize.x) * 0.5f, curPos.y + (thumbHeight - txtSize.y) * 0.5f }, IM_COL32(100, 100, 100, 255), txt);
                        }

                        float metadataBaseY = pad + thumbHeight + 10.0f;
                        ImGui::SetCursorPosY(metadataBaseY);

                        ImGui::BeginGroup();
                        ImGui::Indent(pad);

                        float oneLineHeight = ImGui::GetTextLineHeight();
                        std::string titleChildId = "TitleArea_" + std::to_string(i);

                        if (ImGui::BeginChild(titleChildId.c_str(), ImVec2(contentWidth, oneLineHeight + 2.0f), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
                        {
                            if (m_EditingVideoIndex == i)
                            {
                                ImGui::SetNextItemWidth(contentWidth);
                                ImGui::SetKeyboardFocusHere();
                                if (ImGui::InputText("##edit_video_name", RenameVideoBuf, IM_ARRAYSIZE(RenameVideoBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                                {
                                    g_pSystem->Gallery.RenameVideo(i, RenameVideoBuf);
                                    m_EditingVideoIndex = -1;
                                }
                                if (ImGui::IsItemDeactivated() && !ImGui::IsItemActive()) m_EditingVideoIndex = -1;
                            }
                            else
                            {
                                ImGui::TextUnformatted(video.FileName.c_str());

                                if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(0))
                                {
                                    m_EditingVideoIndex = i;
                                    strncpy_s(RenameVideoBuf, video.FileName.c_str(), sizeof(RenameVideoBuf));
                                }
                            }
                        }
                        ImGui::EndChild();

                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", video.FileName.c_str());

                        ImGui::SetCursorPosY(metadataBaseY + oneLineHeight + 15.0f);

                        if (video.IsMetadataLoaded) 
                        {
                            if (video.Duration > 0.0f) 
                            {
                                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Duration: %s", g_pSystem->Gallery.FormatDuration(video.Duration).c_str());
                            }
                            else 
                            {
                                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Duration: [Recording]");
                            }
                        }
                        else 
                        {
                            ImGui::TextDisabled("Duration: Loading...");
                        }

                        ImGui::TextDisabled("Size: %s", g_pSystem->Gallery.FormatBytes(video.FileSize).c_str());

                        ImGui::Unindent(pad);
                        ImGui::EndGroup();

                        float buttonHeight = ImGui::GetFrameHeight();
                        ImGui::SetCursorPos({ pad, 320.0f - buttonHeight - pad });
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
                        if (ImGui::Button("Delete", ImVec2(contentWidth, 0))) 
                        {
                            m_VideoIndexToDelete = i;
                            m_OpenDeleteVideoModal = true;
                        }
                        ImGui::PopStyleColor();
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor(2);
                    ImGui::PopStyleVar(2);
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::EndChild();
}


void CaptureTab::DrawPopups()
{
    if (m_OpenUninstallModal.load())
    {
        ImGui::OpenPopup("Confirm Uninstall");
        m_OpenUninstallModal.store(false);
    }

    if (ImGui::BeginPopupModal("Confirm Uninstall", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        this->DrawUninstallPopup();
    }

    if (m_OpenDeleteVideoModal.load())
    {
        ImGui::OpenPopup("Delete Video");
        m_OpenDeleteVideoModal.store(false);
    }

    if (ImGui::BeginPopupModal("Delete Video", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        this->DrawDeleteVideoPopup();
    }
}

void CaptureTab::DrawUninstallPopup()
{
    ImGui::Text("Are you sure you want to uninstall FFmpeg?\n"
        "This will disable all recording features.");
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    float buttonWidth = 100.0f;
    float totalButtonWidth = (buttonWidth * 2.0f) + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalButtonWidth) * 0.5f);
    
    if (ImGui::Button("Yes", ImVec2(buttonWidth, 0)))
    {
        g_pSystem->FFmpeg.UninstallFFmpeg();
        ImGui::CloseCurrentPopup();
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
    {
        ImGui::CloseCurrentPopup();
    }
    
    ImGui::EndPopup();
}

void CaptureTab::DrawDeleteVideoPopup()
{
    ImGui::Text("Are you sure you want to delete this video?\nThis action cannot be undone.\n\n");
    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("Yes", ImVec2(120, 0)))
    {
        g_pSystem->Gallery.DeleteVideo(m_VideoIndexToDelete); 
        ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor();

    if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
    ImGui::SameLine();

    ImGui::EndPopup();
}

std::string CaptureTab::OpenFolderDialog() 
{
    std::string folderPath = "";

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return "";

    IFileOpenDialog* pFileOpen;
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr)) 
    {
        DWORD dwOptions;
        pFileOpen->GetOptions(&dwOptions);
        pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);

        hr = pFileOpen->Show(NULL);

        if (SUCCEEDED(hr)) 
        {
            IShellItem* pItem;
            hr = pFileOpen->GetResult(&pItem);

            if (SUCCEEDED(hr)) 
            {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                if (SUCCEEDED(hr)) 
                {
                    int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                    
                    if (size_needed > 0) 
                    {
                        std::vector<char> buffer(size_needed);
                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &buffer[0], size_needed, NULL, NULL);
                        folderPath = std::string(&buffer[0]);
                    }

                    CoTaskMemFree(pszFilePath);
                }

                pItem->Release();
            }
        }

        pFileOpen->Release();
    }

    CoUninitialize();
    return folderPath;
}