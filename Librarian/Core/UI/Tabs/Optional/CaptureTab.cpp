#include "pch.h"
#include "Core/UI/CoreUI.h"
#include "Core/UI/Tabs/Primary/SettingsTab.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Infrastructure/CaptureThread.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Domain/Director/DirectorState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/States/Infrastructure/Capture/DownloadState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/States/Infrastructure/Persistence/GalleryState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Infrastructure/Capture/AudioSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"
#include "Core/Systems/Infrastructure/Capture/DownloadSystem.h"
#include "Core/Systems/Infrastructure/Engine/DialogSystem.h"
#include "Core/Systems/Infrastructure/Engine/FormatSystem.h"
#include "Core/Systems/Infrastructure/Persistence/GallerySystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/UI/Tabs/Optional/CaptureTab.h"
#include "External/imgui/imgui.h"

void CaptureTab::Draw()
{
    bool isRecording = g_pState->Infrastructure->FFmpeg->IsRecording();

	this->DrawTopBar(isRecording);

	ImGui::Separator();

	if (g_pState->Infrastructure->Download->IsFFmpegInstalled())
	{
        this->DrawGallery(isRecording);
	}

    this->DrawRecordingSettingsPopup(isRecording);
    this->DrawPopups();
}

void CaptureTab::DrawTopBar(bool isRecording)
{
    bool isCaptureActive = g_pState->Infrastructure->FFmpeg->IsCaptureActive();
    float totalWidth = ImGui::GetContentRegionAvail().x;

    if (g_pState->Infrastructure->Download->IsFFmpegInstalled())
    {
        // Left side
        this->DrawRecordingControls(isRecording, isCaptureActive);
    }
    else
    {
        ImGui::BeginGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Recording Controls Disabled");
        ImGui::EndGroup();
    }
    
    ImGui::SameLine();

    // Right side
    this->DrawFFmpegControls(isRecording, totalWidth);
}

void CaptureTab::DrawRecordingControls(bool isRecording, bool isCaptureActive)
{
    ImGui::BeginGroup();

    const char* statusText = "WAITING";
    ImVec4 statusColor = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);

    bool stopRequested = g_pState->Infrastructure->FFmpeg->RecordingStopped();

    if (g_pState->Infrastructure->FFmpeg->RecordingStarted() || (isRecording && !isCaptureActive))
    {
        statusText = "STARTING";
        statusColor = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
    }
    else if (isCaptureActive && !stopRequested)
    {
        statusText = "RECORDING";
        statusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    else if (stopRequested && isRecording)
    {
        statusText = "STOPPING";
        statusColor = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("STATUS:");
    ImGui::SameLine();
    ImGui::TextColored(statusColor, statusText);
    ImGui::SameLine(0, 10.0f);

    if (!isRecording)
    {
        m_StopRequestedTime = std::chrono::steady_clock::time_point();

        bool canRecord = g_pState->Domain->Theater->IsTheaterMode() &&
            g_pState->Domain->Theater->GetTimePtr() != nullptr;

        if (!canRecord) ImGui::BeginDisabled();

        if (ImGui::Button("Start Recording"))
        {
            g_pThread->Capture->StartRecording();
        }

        if (!canRecord)
        {
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::BeginTooltip();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Recording unavailable:");
                if (!g_pState->Domain->Theater->IsTheaterMode())             ImGui::BulletText("Theater Mode is not active");
                if (g_pState->Domain->Theater->GetTimePtr() == nullptr)      ImGui::BulletText("Theater Time not found");
                ImGui::EndTooltip();
            }
            ImGui::EndDisabled();
        }
    }
    else
    {
        if (!stopRequested)
        {
            if (!isCaptureActive) ImGui::BeginDisabled();

            const char* btnLabel = !isCaptureActive ? "  ...  " : "Stop Recording";
            if (ImGui::Button(btnLabel))
            {
                g_pThread->Capture->StopRecording();
                m_StopRequestedTime = std::chrono::steady_clock::now();
            }

            if (!isCaptureActive && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::SetTooltip("Please wait while AutoTheater initializes FFmpeg and related systems...");
            }
            if (!isCaptureActive) ImGui::EndDisabled();
        }
        else
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_StopRequestedTime).count();

            if (elapsed < 1000)
            {
                ImGui::BeginDisabled();
                ImGui::Button("Stopping...  ");
                ImGui::EndDisabled();
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));

                if (ImGui::Button("Force Stop"))
                {
                    g_pThread->Capture->StopRecording(true);
                    g_pSystem->Debug->Log("[CaptureTab] INFO: Force stop request detected from UI.");
                }

                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Emergency stop.");
            }
        }
    }

    ImGui::SameLine(0, 10.0f);
    if (isCaptureActive)
    {
        ImGui::TextDisabled("Time:");
        ImGui::SameLine();
        float recordingDuration = g_pSystem->Infrastructure->FFmpeg->GetRecordingDuration();
        ImGui::TextUnformatted(g_pSystem->Infrastructure->Format->ToTimestamp(recordingDuration).c_str());
    }

    ImGui::EndGroup();
}

void CaptureTab::DrawFFmpegControls(bool isRecoring, float totalWidth)
{
    ImGui::BeginGroup();

    float padding = ImGui::GetStyle().ItemSpacing.x;

    if (g_pState->Infrastructure->Download->IsDownloadInProgress())
    {
        float barW = 180.0f;
        float btnW = ImGui::CalcTextSize("Cancel").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float groupW = barW + padding + btnW;

        ImGui::SetCursorPosX(totalWidth - groupW - padding);

        float progress = g_pState->Infrastructure->Download->GetDownloadProgress() / 100.0f;

        std::string progressPercent = std::to_string(static_cast<int>(progress * 100.0f)) + "%";

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));

        ImGui::SetNextItemWidth(barW);
        ImGui::ProgressBar(progress, ImVec2(0, 0), progressPercent.c_str());

        ImGui::PopStyleColor();
        ImGui::SameLine();

        bool isFinished = (progress >= 1.0f);
        if (isFinished) ImGui::BeginDisabled();

        if (ImGui::Button("Cancel"))
        {
            g_pSystem->Infrastructure->Download->CancelDownload();
        }

        if (isFinished) ImGui::EndDisabled();
    }
    else if (g_pState->Infrastructure->Download->IsFFmpegInstalled())
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
                ImGui::SetTooltip("Cannot uninstall FFmpeg while recording is in progress.\n"
                    "Stop the current session before removing dependencies.");
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
            g_pSystem->Infrastructure->Download->DownloadDependencies();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("FFmpeg is required for recording and processing videos.");
        }
    }
    ImGui::EndGroup();
}


void CaptureTab::DrawRecordingSettingsPopup(bool isRecording)
{
    if (m_OpenRecordingSettingsModal.load())
    {
        ImGui::OpenPopup("Recording Settings");
        m_OpenRecordingSettingsModal.store(false);
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    ImGui::SetNextWindowSize(ImVec2(550, 400), ImGuiCond_Appearing);

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0, 0, 0, 0.5f));

    bool open = true;
    if (ImGui::BeginPopupModal("Recording Settings", &open, ImGuiWindowFlags_NoSavedSettings))
    {
        this->DrawRecordingSettings(isRecording);

        if (!open) ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor();
}

void CaptureTab::DrawRecordingSettings(bool isRecording)
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (!m_PendingNewPath.empty())
        {
            g_pState->Infrastructure->FFmpeg->SetOutputPath(m_PendingNewPath);
            g_pSystem->Infrastructure->Gallery->RefreshList(m_PendingNewPath);
            m_PendingNewPath = "";
        }
    }

    auto DrawSectionCard = [&](const char* header, std::function<void()> content, bool defaultOpen = false) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;
        if (defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

        if (ImGui::CollapsingHeader(header, flags))
        {
            ImGui::Indent(10.0f);

            ImVec2 p_min = ImGui::GetCursorScreenPos();
            p_min.x -= 5.0f;

            ImGui::BeginGroup();
            ImGui::Spacing();
            ImGui::Indent(10.0f);

            content();

            ImGui::Spacing();
            ImGui::Unindent(10.0f);
            ImGui::EndGroup();

            ImVec2 p_max = ImVec2(p_min.x + ImGui::GetItemRectSize().x + 10.0f,
                p_min.y + ImGui::GetItemRectSize().y);

            ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, ImColor(255, 255, 255, 15), 5.0f);
            ImGui::GetWindowDrawList()->AddRect(p_min, p_max, ImColor(255, 255, 255, 30), 5.0f);

            ImGui::Unindent(10.0f);
            ImGui::Spacing();
        }
    };

    auto encoderConfig = g_pState->Infrastructure->FFmpeg->GetEncoderConfig();
    bool configChanged = false;

    DrawSectionCard("Video Encoding", [&]() {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Output Quality");

        if (isRecording) ImGui::BeginDisabled();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Resolution Scale");
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 205.0f);
        ImGui::SetNextItemWidth(205.0f);

        const char* resolutions[] = { "1080p", "1440p", "2160p" };
        int currentRes = (int)g_pState->Infrastructure->FFmpeg->GetResolutionType();

        if (ImGui::Combo("##ResCombo", &currentRes, resolutions, IM_ARRAYSIZE(resolutions)))
        {
            g_pState->Infrastructure->FFmpeg->SetResolutionType((ResolutionType)currentRes);
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

        ImGui::Spacing();

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Target Framerate");
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 205.0f);
        ImGui::SetNextItemWidth(205.0f);

        ImGui::BeginDisabled();
        const char* fpsOptions[] = { "60 FPS", "120 FPS", "180 FPS", "240 FPS" };
        float currentFPS = g_pState->Infrastructure->FFmpeg->GetTargetFramerate();
        int fpsIdx = (currentFPS >= 240) ? 3 : (currentFPS >= 180) ? 2 : (currentFPS >= 120) ? 1 : 0;

        if (ImGui::BeginCombo("##FPSCombo", fpsOptions[fpsIdx])) 
        {
            for (int n = 0; n < IM_ARRAYSIZE(fpsOptions); n++) 
            {
                if (ImGui::Selectable(fpsOptions[n], fpsIdx == n)) 
                {
                    float val = (n == 0) ? 60.0f : (n == 1) ? 120.0f : (n == 2) ? 180.0f : 240.0f;
                    g_pState->Infrastructure->FFmpeg->SetTargetFramerate(val);
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Framerate Synchronization");
            ImGui::Spacing();
            ImGui::Text("Currently managed by in-game settings.");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("The recording system requires a stable framerate to maintain\n"
                "audio/video sync through the FFmpeg pipes.");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Important:");
            ImGui::Text("Recording is disabled if 'Unlimited FPS' is selected on the game settings.\n"
                "Variable framerates can cause the recording to fail or desync.");

            ImGui::EndTooltip();
        }
        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Encoder Settings");

        const char* encoders[] = { "NVIDIA NVENC", "AMD AMF", "Intel QuickSync", "Software (CPU)" };
        int currentEncoder = (int)encoderConfig.EncoderType;
        if (ImGui::Combo("Hardware Encoder", &currentEncoder, encoders, IM_ARRAYSIZE(encoders)))
        {
            encoderConfig.EncoderType = (EncoderType)currentEncoder;
            configChanged = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Video Encoding Engine");
            ImGui::Separator();

            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Hardware (Recommended):");
            ImGui::BulletText("NVENC (NVIDIA): Top tier quality/speed.");
            ImGui::BulletText("AMF (AMD): Optimized for Radeon.");
            ImGui::BulletText("QuickSync (Intel): Great iGPU fallback.");

            ImGui::Spacing();

            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Software (CPU):");
            ImGui::Text("Uses x264. Very taxing. High resolutions may cause stutters or crashes.");

            ImGui::Separator();
            ImGui::TextDisabled("Note: Selection must match your hardware.");

            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        const char* containers[] = { "Matroska (.mkv)", "MPEG-4 (.mp4)" };
        int currentContainer = (int)encoderConfig.OutputContainer;
        if (ImGui::Combo("File Format", &currentContainer, containers, IM_ARRAYSIZE(containers))) {
            encoderConfig.OutputContainer = (OutputContainer)currentContainer;
            configChanged = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Output Container Format");
            ImGui::Spacing();
            ImGui::Text("Choose the file wrapper for your video tracks.");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::BulletText("MKV (Recommended): Crash-resistant. If the game or PC crashes,"
                " the video remains playable up to the last second recorded.");
            ImGui::BulletText("MP4: Industry standard, widely compatible with all players."
                " However, it is NOT crash-safe.");

            ImGui::EndTooltip();
        }

        int bitrateMbps = encoderConfig.BitrateKbps / 1000;
        if (ImGui::SliderInt("Bitrate (Mbps)", &bitrateMbps, 20, 150))
        {
            if (bitrateMbps < 20) bitrateMbps = 20;
            else if (bitrateMbps > 150) bitrateMbps = 150;

            encoderConfig.BitrateKbps = bitrateMbps * 1000;
            configChanged = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Video Bitrate Control");
            ImGui::Spacing();
            ImGui::Text("Controls the amount of data processed per second.");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::BulletText("20 - 50 Mbps: Good for 1080p and general sharing.");
            ImGui::BulletText("50 - 100 Mbps: High fidelity, recommended for 1440p.");
            ImGui::BulletText("100 - 150 Mbps: Near-lossless. Best for 4K or further editing.");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Warning: Higher bitrates significantly increase file size.");
            ImGui::EndTooltip();
        }

        const char* presets[] = { "P1 (Fastest)", "P2", "P3", "P4 (Balanced)", "P5", "P6", "P7 (Slowest/High Quality)" };
        int currentPreset = (int)encoderConfig.VideoPreset;
        if (ImGui::Combo("Encoder Preset", &currentPreset, presets, IM_ARRAYSIZE(presets)))
        {
            encoderConfig.VideoPreset = (VideoPreset)currentPreset;
            configChanged = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Encoder Efficiency Preset");
            ImGui::Spacing();
            ImGui::Text("Determines the trade-off between encoding speed and visual quality.");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::BulletText("P1 - P3: Optimized for performance. Use if you experience lag.");
            ImGui::BulletText("P4: Balanced setting. Recommended for most systems.");
            ImGui::BulletText("P5 - P7: Maximum quality. Better detail retention at the same bitrate,"
                " but requires more GPU resources.");
            
            ImGui::EndTooltip();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Performance & Buffering");

        if (ImGui::SliderInt("Audio Buffered Packets", &encoderConfig.MaxAudioBufferedPackets, 128, 2048))
        {
            encoderConfig.MaxAudioBufferedPackets = 
                std::clamp(encoderConfig.MaxAudioBufferedPackets, 128, 2048);

            configChanged = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::BeginTooltip();
            ImGui::Text("Sets the FFmpeg '-thread_queue_size' for the Audio stream.");
            ImGui::Spacing();

            ImGui::BulletText("Defines the maximum number of PCM audio packets stored in RAM\n"
                "before FFmpeg starts dropping them.");

            ImGui::BulletText("At 48kHz, 1024 packets provide a massive safety buffer\n"
                "against encoding delays with minimal RAM impact.");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                "Tip: Increase this if you notice audio crackling or desync\n"
                "during high CPU load or when recording at 4K.");

            ImGui::EndTooltip();
        }

        if (ImGui::SliderInt("Video Buffered Frames", &encoderConfig.MaxBufferedFrames, 60, 512))
        {
            encoderConfig.MaxBufferedFrames = std::clamp(encoderConfig.MaxBufferedFrames, 60, 512);
            configChanged = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Video Frame Buffer Pool");
            ImGui::Separator();

            ImGui::Text("Sets the number of raw frames kept in RAM as a safety cushion.");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Buffer Capacity (at 60 FPS):");
            ImGui::BulletText("60 Frames:  ~1.0 seconds of safety.");
            ImGui::BulletText("120 Frames: ~2.0 second of safety.");
            ImGui::BulletText("240 Frames: ~4.0 seconds of safety.");

            ImGui::Spacing();
            ImGui::Text("A larger pool prevents 'Pipe Timeout' errors by allowing the game "
                "to keep capturing even if the disk or GPU encoder has a momentary spike.");

            ImGui::Separator();

            int width = g_pState->Infrastructure->Render->GetWidth();
            int height = g_pState->Infrastructure->Render->GetHeight();
            float estRAM = (width * height * 4.0f * encoderConfig.MaxBufferedFrames) / (1024.0f * 1024.0f);

            ImGui::TextDisabled("Estimated RAM Usage: %.0f MB at %dx%d", estRAM, width, height);
            ImGui::TextDisabled("Note: Memory is pre-allocated when recording starts.");

            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        const char* filters[] = { "Bicubic (Fast)", "Lanczos (Sharp)", "Bilinear (Light)", "Spline" };
        int currentFilter = (int)encoderConfig.ScalingFilter;
        if (ImGui::Combo("Scaling Filter", &currentFilter, filters, IM_ARRAYSIZE(filters)))
        {
            encoderConfig.ScalingFilter = (ScalingFilter)currentFilter;
            configChanged = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Resampling Algorithm");
            ImGui::Spacing();
            ImGui::Text("Determines how frames are resized if the output resolution"
                " differs from your current game window.");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::BulletText("Bicubic: Balanced speed and quality (Recommended).");
            ImGui::BulletText("Lanczos: High quality, sharper results, more CPU intensive.");
            ImGui::BulletText("Bilinear: Fastest, but can look a bit blurry.");
            ImGui::BulletText("Spline: Excellent for high-ratio upscaling.");

            ImGui::Spacing();
            ImGui::TextDisabled("Note: If your in-game resolution is the same as the selected"
                " output resolution scale, this filter is bypassed entirely.");
            ImGui::EndTooltip();
        }

        if (isRecording) ImGui::EndDisabled();
    }, false);

    DrawSectionCard("Recording Behaviour", [&]() {
        if (isRecording) ImGui::BeginDisabled();

        bool recordOverlay = g_pState->Infrastructure->FFmpeg->ShouldRecordUI();
        if (ImGui::Checkbox("Record ImGui Overlay", &recordOverlay))
        {
            g_pState->Infrastructure->FFmpeg->SetRecordUI(recordOverlay);
        }

        bool muted = g_pState->Infrastructure->Audio->IsMuted();
        if (ImGui::Checkbox("Record muted", &muted))
        {
            g_pState->Infrastructure->Audio->SetMuted(muted);
        }

        bool stopOnLast = g_pState->Infrastructure->FFmpeg->StopOnLastEvent();
        if (ImGui::Checkbox("Stop on last event", &stopOnLast))
        {
            g_pState->Infrastructure->FFmpeg->SetStopOnLastEvent(stopOnLast);
        }

        bool shouldDisableSlider = !stopOnLast;
        if (shouldDisableSlider) ImGui::BeginDisabled();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Post-roll Delay");
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 205.0f);
        ImGui::SetNextItemWidth(205.0f);

        float stopDelay = g_pState->Infrastructure->FFmpeg->GetStopDelayDuration();
        if (ImGui::SliderFloat("##DelaySlider", &stopDelay, 0.0f, 20.0f, "%.1fs"))
        {
            g_pState->Infrastructure->FFmpeg->SetStopDelayDuration(stopDelay);
        }

        if (shouldDisableSlider) ImGui::EndDisabled();

        if (isRecording) ImGui::EndDisabled();
    });

    DrawSectionCard("Output & Storage", [&]() {
        if (isRecording) ImGui::BeginDisabled();

        std::string outputPath = g_pState->Infrastructure->FFmpeg->GetOutputPath();

        g_pUI->Settings->DrawPathField("Output Directory", outputPath, 110.0f);

        ImGui::SameLine();

        if (ImGui::Button("Browse", ImVec2(100, 0)))
        {
            m_FolderPickerActive.store(true);

            std::thread([this]() {
                std::string selectedPath = g_pSystem->Infrastructure->Dialog->OpenFolderDialog();
                if (!selectedPath.empty())
                {
                    std::lock_guard<std::mutex> lock(m_Mutex);
                    this->m_PendingNewPath = selectedPath;
                }

                this->m_FolderPickerActive.store(false);
            }).detach();
        }

        if (isRecording) ImGui::EndDisabled();
    });

    if (configChanged) g_pState->Infrastructure->FFmpeg->UpdateEncoderConfig(encoderConfig);
}   

void CaptureTab::DrawGallery(bool isRecording)
{
    const std::vector<VideoData> videos = g_pState->Infrastructure->Gallery->GetVideos();
    int videoCount = (int)videos.size();

    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Recorded Videos (%d)", videoCount);

    size_t pending = g_pSystem->Infrastructure->Gallery->GetPendingCount();
    if (pending > 0) 
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "  (Processing: %zu left)", pending);
    }

    float btn1Width = 155.0f;
    float btn2Width = 125.0f;
    float spacing = 10.0f;
    float totalButtonsWidth = btn1Width + spacing + btn2Width;

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - totalButtonsWidth + ImGui::GetCursorPosX());

    if (ImGui::Button("Recording Settings", ImVec2(btn1Width, 0)))
    {
        m_OpenRecordingSettingsModal.store(true);
    }

    ImGui::SameLine(0, spacing);

    if (isRecording) ImGui::BeginDisabled();
    if (ImGui::Button("Refresh Gallery", ImVec2(btn2Width, 0)))
    {
        g_pSystem->Infrastructure->Gallery->RefreshList(g_pState->Infrastructure->FFmpeg->GetOutputPath());
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
                        g_pState->Infrastructure->Gallery->SetLoading(i, true);
                        g_pSystem->Infrastructure->Gallery->LoadMetadataAsync(i);
                    }

                    ImGui::TableNextColumn();
                    ImGui::PushID(i);

                    float cellWidth = ImGui::GetContentRegionAvail().x;
                    bool isSelected = (g_pState->Infrastructure->Gallery->GetSelectedIndex() == i);

                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, isSelected ? ImVec4(0.18f, 0.22f, 0.30f, 1.0f) : ImVec4(0.12f, 0.12f, 0.12f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_Border, isSelected ? ImVec4(0.40f, 0.60f, 1.0f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

                    if (ImGui::BeginChild("VideoCard", ImVec2(cellWidth, 320), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
                    {
                        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
                            g_pState->Infrastructure->Gallery->SetSelectedIndex(i);

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

                        float titleAreaHeight = oneLineHeight + 8.0f;

                        if (ImGui::BeginChild(titleChildId.c_str(), ImVec2(contentWidth, titleAreaHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
                        {
                            if (m_EditingVideoIndex == i)
                            {
                                ImGui::SetNextItemWidth(contentWidth);
                                ImGui::SetKeyboardFocusHere();

                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 2));
                                if (ImGui::InputText("##edit_video_name", RenameVideoBuf, IM_ARRAYSIZE(RenameVideoBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                                {
                                    g_pSystem->Infrastructure->Gallery->RenameVideo(i, RenameVideoBuf);
                                    m_EditingVideoIndex = -1;
                                }
                                ImGui::PopStyleVar();

                                if (ImGui::IsItemDeactivated() && !ImGui::IsItemActive()) m_EditingVideoIndex = -1;
                            }
                            else
                            {
                                std::string displayName = video.FileName;
                                size_t lastDot = displayName.find_last_of(".");
                                if (lastDot != std::string::npos)
                                {
                                    displayName = displayName.substr(0, lastDot);
                                }

                                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
                                ImGui::TextUnformatted(displayName.c_str());

                                if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(0))
                                {
                                    m_EditingVideoIndex = i;
                                    strncpy_s(RenameVideoBuf, displayName.c_str(), sizeof(RenameVideoBuf));
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
                                ImGui::TextDisabled("Duration: %s", g_pSystem->Infrastructure->Gallery->FormatDuration(video.Duration).c_str());
                            }
                            else 
                            {
                                ImGui::TextDisabled("Duration: --:--:--");
                            }
                        }
                        else 
                        {
                            ImGui::TextDisabled("Duration: Loading...");
                        }

                        ImGui::TextDisabled("Size: %s", g_pSystem->Infrastructure->Gallery->FormatBytes(video.FileSize).c_str());

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
        ImGui::EndPopup();
    }

    if (m_OpenDeleteVideoModal.load())
    {
        ImGui::OpenPopup("Delete Video");
        m_OpenDeleteVideoModal.store(false);
    }

    if (ImGui::BeginPopupModal("Delete Video", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        this->DrawDeleteVideoPopup();
        ImGui::EndPopup();
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
        g_pSystem->Infrastructure->Download->UninstallDependencies();
        ImGui::CloseCurrentPopup();
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
    {
        ImGui::CloseCurrentPopup();
    }
}

void CaptureTab::DrawDeleteVideoPopup()
{
    ImGui::Text("Are you sure you want to delete this video?\nThis action cannot be undone.\n\n");
    ImGui::Separator();

    float buttonWidth = 120.0f;

    float totalWidth = (buttonWidth * 2.0f) + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalWidth) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("Yes", ImVec2(buttonWidth, 0)))
    {
        g_pSystem->Infrastructure->Gallery->DeleteVideo(m_VideoIndexToDelete);
        ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
    {
        ImGui::CloseCurrentPopup();
    }
<<<<<<< HEAD
=======
}

void CaptureTab::DrawTelemetryPopup()
{
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(450, 300), ImVec2(800, 800));

    bool open = true;
    if (ImGui::BeginPopupModal("Telemetry", &open, ImGuiWindowFlags_NoSavedSettings))
    {
        if (!open) ImGui::CloseCurrentPopup();

        g_pSystem->Infrastructure->FFmpeg->UpdateQueueTelemetry();

        CaptureTelemetry telemetry = g_pSystem->Infrastructure->FFmpeg->GetTelemetry();

        static double smoothedRatio = 1.0;
        double rawRatio = g_pThread->Capture->GetSyncRatio();
        smoothedRatio = 0.05 * rawRatio + 0.95 * smoothedRatio;

        ImGui::TextDisabled("General");
        ImGui::Separator();

        if (ImGui::BeginTable("##EncoderTable", 2, ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Encoding Speed");
            ImGui::TableSetColumnIndex(1);

            ImVec4 speedCol = (telemetry.FFmpegSpeed < 0.98f) ? 
                ImVec4(1, 0.2f, 0.2f, 1) : ImVec4(0, 1, 0, 1);

            ImGui::TextColored(speedCol, "%.2fx", telemetry.FFmpegSpeed);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Sync Ratio");
            ImGui::TableSetColumnIndex(1);

            ImVec4 ratioCol = (smoothedRatio < 0.999) ? 
                ImVec4(1, 0.2f, 0.2f, 1) : ImVec4(1, 1, 1, 1);

            ImGui::TextColored(ratioCol, "%.3f", smoothedRatio);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Bitrate");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.0f kbps", telemetry.CurrentBitrateKbps);

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Video & Audio Buffers");
        ImGui::Separator();

        if (ImGui::BeginTable("##QueuesTable", 2, ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Pending Frames");
            ImGui::TableSetColumnIndex(1);

            float vMaxFrames = 64.0f;
            float vFill = (std::min)((float)telemetry.VideoPendingQueueSize / vMaxFrames, 1.0f);

            ImVec4 vColor = (vFill > 0.8f) ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f) :
                (vFill > 0.5f) ? ImVec4(1.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.2f, 0.7f, 0.2f, 1.0f);

            std::string progressText = std::to_string(telemetry.VideoPendingQueueSize) +
                (telemetry.VideoPendingQueueSize == 1 ? " frame" : " frames");

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, vColor);
            ImGui::ProgressBar(vFill, ImVec2(-FLT_MIN, 0), progressText.c_str());
            ImGui::PopStyleColor();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Pending Chunks");
            ImGui::TableSetColumnIndex(1);

            float aMaxChunks = 500.0f;
            float aFill = (std::min)((float)telemetry.AudioPendingQueueSize / aMaxChunks, 1.0f);

            ImVec4 aColor = (aFill > 0.8f) ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f) :
                (aFill > 0.5f) ? ImVec4(1.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.2f, 0.7f, 0.2f, 1.0f);
            
            std::string audioStatus = std::to_string(telemetry.AudioPendingQueueSize) +
                (telemetry.AudioPendingQueueSize == 1 ? " chunk" : " chunks");

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, aColor);
            ImGui::ProgressBar(aFill, ImVec2(-FLT_MIN, 0), audioStatus.c_str());
            ImGui::PopStyleColor();

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Memory Pool");
        ImGui::Separator();

        if (ImGui::BeginTable("##PoolTable", 2, ImGuiTableFlags_SizingStretchSame))
        {
            int poolCurrent = g_pSystem->Infrastructure->Video->GetPoolSize();
            int poolMax = g_pSystem->Infrastructure->Video->GetPoolMaxSize() - 1;
            size_t bufferSize = g_pSystem->Infrastructure->Video->GetBufferSize();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Pool Buffers");
            ImGui::TableSetColumnIndex(1);

            float poolFill = (poolMax > 0) ? (float)poolCurrent / (float)poolMax : 0.0f;
            ImVec4 poolColor = (poolFill < 0.1f) ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f) :
                (poolFill < 0.3f) ? ImVec4(1.0f, 0.6f, 0.0f, 1.0f) :
                ImVec4(0.2f, 0.7f, 0.2f, 1.0f);

            std::string poolText = std::to_string(poolCurrent) + " / " + std::to_string(poolMax);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, poolColor);
            ImGui::ProgressBar(poolFill, ImVec2(-FLT_MIN, 0), poolText.c_str());
            ImGui::PopStyleColor();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Frame Size");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.1f MB", bufferSize / (1024.0f * 1024.0f));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Total Pool RAM");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.0f MB", (poolCurrent * bufferSize) / (1024.0f * 1024.0f));

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Write Latency");
        ImGui::Separator();

        if (ImGui::BeginTable("##PipesTable", 2, ImGuiTableFlags_SizingStretchSame))
        {
            auto DrawLatencyRow = [](const char* label, float current, float peak, float warn, float crit) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0); ImGui::Text("%s", label);
                ImGui::TableSetColumnIndex(1);

                ImVec4 color = (current > crit) ? ImVec4(1, 0.2f, 0.2f, 1) : (current > warn) ? ImVec4(1, 1, 0, 1) : ImVec4(1, 1, 1, 1);

                ImGui::TextColored(color, "%.1f ms", current);
                ImGui::SameLine(); 
                ImGui::TextDisabled("(Peak: %.1f ms)", peak);
            };

            DrawLatencyRow("Video", telemetry.LastVideoWriteLatencyMs, telemetry.MaxVideoWriteLatencyMs, 15.0f, 50.0f);
            DrawLatencyRow("Audio", telemetry.LastAudioWriteLatencyMs, telemetry.MaxAudioWriteLatencyMs, 10.0f, 20.0f);

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
>>>>>>> 1ad5498357ea365b2a611b850ddc8cb4825f820f
}