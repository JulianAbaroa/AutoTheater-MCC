#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/Hooks/Memory/TargetFramerateHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Director/DirectorState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"
#include "Core/Systems/Infrastructure/Capture/AudioSystem.h"
#include "Core/Systems/Infrastructure/Persistence/GallerySystem.h"
#include "Core/Threads/Infrastructure/CaptureThread.h"

// TODO: It seems recording at 240 desynchronizes the video & audio.

using namespace std::chrono_literals;

void CaptureThread::Run()
{
    g_pUtil->Log.Append("[CaptureThread] INFO: Started.");

    while (g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        // Filters.
        this->VerifyAndPrepareFFmpeg();
        if (!this->ReadyToCapture())
        {
            std::this_thread::sleep_for(10ms);
            continue;
        }

        // Start.
        if (g_pState->Infrastructure->FFmpeg->RecordingStarted() && 
            !g_pState->Infrastructure->FFmpeg->IsRecording())
        {
            if (g_pState->Infrastructure->Audio->GetMasterInstance() != nullptr)
            {
                this->StartRecording();
            }
            else
            {
                std::this_thread::sleep_for(1ms);
                continue;
            }
        }

        if (g_pState->Infrastructure->FFmpeg->IsRecording())
        {
            if (this->VideoQueueOverflow()) continue;

            int currentFramerate = g_pHook->Memory->TargetFramerate->GetCurrentFramerateValue();
            if (currentFramerate == 0.0f)
            {
                g_pSystem->Infrastructure->FFmpeg->ForceStop();
                g_pState->Infrastructure->FFmpeg->SetTargetFramerate(static_cast<float>(currentFramerate));
                g_pUtil->Log.Append("[CaptureThread] ERROR: AutoTheater cannot record with UNLIMITED framerate active, recording stopped.");
                continue;
            }

            int targetFramerate = static_cast<int>(g_pState->Infrastructure->FFmpeg->GetTargetFramerate());
            
            bool wasFramerateChanged = (currentFramerate != targetFramerate);
            if (g_pState->Infrastructure->FFmpeg->RecordingStopped() || wasFramerateChanged)
            {
                if (wasFramerateChanged)
                {
                    g_pSystem->Infrastructure->FFmpeg->ForceStop();
                    g_pState->Infrastructure->FFmpeg->SetTargetFramerate(static_cast<float>(currentFramerate));

                    g_pUtil->Log.Append("[CaptureThread] WARNING: Recording stopped, the game framerate"
                        " was changed from %d to %d FPS.", targetFramerate, currentFramerate);
                }
                else
                {
                    g_pSystem->Infrastructure->FFmpeg->Stop();
                }

                g_pSystem->Infrastructure->Gallery->RefreshList(g_pState->Infrastructure->FFmpeg->GetOutputPath());
                continue;
            }

            if (!m_SyncInitialized)
            {
                if (g_pSystem->Infrastructure->Video->GetQueueSize() > 0)
                {
                    auto audioQueue = g_pSystem->Infrastructure->Audio->ExtractQueue();
                    auto videoQueue = g_pSystem->Infrastructure->Video->ExtractQueue();

                    if (this->CaptureBaselineEstablished(audioQueue, videoQueue))
                    {
                        this->ProcessSynchronizedStreams(audioQueue, videoQueue);
                    }
                }

                std::this_thread::sleep_for(1ms);
                continue;
            }

            bool dataProcessed = false;
            while (true)
            {
                auto audio = g_pSystem->Infrastructure->Audio->ExtractQueue();
                auto video = g_pSystem->Infrastructure->Video->ExtractQueue();

                if (audio.empty() && video.empty()) break;

                this->ProcessSynchronizedStreams(audio, video);
                dataProcessed = true;
            }

            if (!dataProcessed) std::this_thread::sleep_for(1ms);
        }
        else
        {
            std::this_thread::sleep_for(10ms);
        }
    }

    if (g_pState->Infrastructure->FFmpeg->IsRecording()) g_pSystem->Infrastructure->FFmpeg->ForceStop();
    g_pUtil->Log.Append("[CaptureThread] INFO: Stopped.");
}


void CaptureThread::VerifyAndPrepareFFmpeg()
{
    if (g_pState->Infrastructure->Settings->ShouldUseAppData() &&
        !g_pState->Infrastructure->FFmpeg->IsFFmpegInstalled() &&
        !g_pState->Infrastructure->FFmpeg->IsDownloadInProgress())
    {
        g_pSystem->Infrastructure->FFmpeg->InitializeFFmpeg();
    }
}

// TODO: See if the capture mode can work on any phase (Default & Timeline)
bool CaptureThread::ReadyToCapture()
{
    return  g_pState->Domain->Theater->IsTheaterMode() &&
            g_pState->Domain->Director->IsInitialized() &&
            g_pState->Domain->Theater->GetTimePtr() != nullptr;
}


void CaptureThread::StartRecording()
{
    g_pState->Infrastructure->FFmpeg->SetStartRecording(false);

    // Filter: Avoid unlimited framerate.
    int currentFramerate = g_pHook->Memory->TargetFramerate->GetCurrentFramerateValue();
    if (currentFramerate == 0.0f)
    {
        g_pState->Infrastructure->FFmpeg->SetTargetFramerate(static_cast<float>(currentFramerate));
        g_pUtil->Log.Append("[CaptureThread] ERROR: AutoTheater cannot record with UNLIMITED framerate active, start recording cancelled.");
        return;
    }

    // Update the target framerate.
    int targetFramerate = static_cast<int>(g_pState->Infrastructure->FFmpeg->GetTargetFramerate());
    if (currentFramerate != targetFramerate)
    {
        g_pState->Infrastructure->FFmpeg->SetTargetFramerate(static_cast<float>(currentFramerate));
        g_pUtil->Log.Append("[CaptureThread] WARNING: Detected framerate change from %d to %d,"
            " recording at %d FPS.", targetFramerate, currentFramerate, currentFramerate);
    }

    bool isStarted = g_pSystem->Infrastructure->FFmpeg->Start(
        g_pState->Infrastructure->FFmpeg->GetOutputPath(),
        g_pState->Infrastructure->Render->GetWidth(), 
        g_pState->Infrastructure->Render->GetHeight(),
        static_cast<float>(currentFramerate)
    );

    if (isStarted)
    {
        this->Cleanup();
        g_pSystem->Infrastructure->Video->StartRecording();
        g_pSystem->Infrastructure->Audio->StartRecording();
    }
    else g_pUtil->Log.Append("[CaptureThread] ERROR: Start recording failed.");
}

void CaptureThread::StopRecording() 
{
    g_pState->Infrastructure->FFmpeg->SetStopRecording(false);
    g_pUtil->Log.Append("[CaptureThread] INFO: Recording stopped.");
}


bool CaptureThread::VideoQueueOverflow()
{
    if (g_pSystem->Infrastructure->Video->GetQueueSize() <= 64) return false;

    if (!m_SyncInitialized)
    {
        g_pSystem->Infrastructure->Video->ClearQueue();
        g_pSystem->Infrastructure->Audio->ClearQueue();
        return true;
    }
    else
    {
        g_pUtil->Log.Append("[CaptureThread] ERROR: Frame queue overload, aborting recording.");
        g_pSystem->Infrastructure->FFmpeg->ForceStop();
        return true;
    }
}

void CaptureThread::MonitorRecordingHealth()
{
    if (!g_pState->Infrastructure->FFmpeg->IsRecording()) return;

    HANDLE hProcess = g_pState->Infrastructure->FFmpeg->GetProcessHandle();
    if (hProcess == NULL) return;

    DWORD exitCode = 0;
    if (hProcess && GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE)
    {
        g_pUtil->Log.Append("[CaptureThread] ERROR: FFmpeg process died. Exit code: %lu. Restarting.", exitCode);
        this->StopRecording();
        std::this_thread::sleep_for(500ms);
        g_pState->Infrastructure->FFmpeg->SetStartRecording(true);
        g_pUtil->Log.Append("[CaptureThread] WARNING: Recovery triggered, starting new segment.");
    }
}


bool CaptureThread::CaptureBaselineEstablished(std::deque<AudioChunk>& audioQueue, std::deque<FrameData>& videoQueue)
{
    bool ready = false;
    std::string mode = "";

    if (!audioQueue.empty() && !videoQueue.empty())
    {
        ready = true;
        mode = "Hybrid";
    }
    else if (!audioQueue.empty() && g_pState->Infrastructure->Audio->GetMasterInstance() != nullptr)
    {
        ready = true;
        mode = "Audio Priority";
    }

    if (ready)
    {
        m_RecordingStartTime = std::chrono::steady_clock::now();
        g_pState->Infrastructure->FFmpeg->SetStartRecordingTime(std::chrono::steady_clock::now());

        m_TotalVideoFramesWritten = 0;
        m_TotalAudioSamplesWritten = 0;
        m_SyncInitialized = true;

        if (!videoQueue.empty()) m_LastFrameBuffer = videoQueue.back().buffer;

        void* masterInstance = g_pState->Infrastructure->Audio->GetMasterInstance();
        if (masterInstance != nullptr)
        {
            m_MasterFormat = g_pState->Infrastructure->Audio->GetAudioInstance(masterInstance);
        }

        g_pUtil->Log.Append("[CaptureThread] INFO: Sync Established (%s)", mode);

        return true;
    }

    return false;
}

void CaptureThread::ProcessSynchronizedStreams(std::deque<AudioChunk>& audioQueue, std::deque<FrameData>& videoQueue)
{
    if (m_MasterFormat.BytesPerFrame == 0 || m_MasterFormat.SamplesPerSec == 0) return;

    if (!m_SyncTimeInitialized)
    {
        m_SyncTimeInitialized = true;
        g_pState->Infrastructure->FFmpeg->SetCaptureActive(true);
    }

    auto now = std::chrono::steady_clock::now();
    float elapsedRealTime = std::chrono::duration<float>(now - m_RecordingStartTime).count();
    
    uint64_t expectedFrames = static_cast<uint64_t>(elapsedRealTime * g_pState->Infrastructure->FFmpeg->GetTargetFramerate());
    uint64_t expectedSamples = static_cast<uint64_t>(elapsedRealTime * m_MasterFormat.SamplesPerSec);

    for (auto& frame : videoQueue)
    {
        if (m_TotalVideoFramesWritten <= expectedFrames)
        {
            g_pSystem->Infrastructure->FFmpeg->WriteVideo(frame.buffer.data(), frame.buffer.size());
            m_TotalVideoFramesWritten++;
        }

        g_pSystem->Infrastructure->Video->ReturnBuffer(std::move(frame.buffer));
    }

    for (auto& chunk : audioQueue)
    {
        if (chunk.Data.size() > 0)
        {
            g_pSystem->Infrastructure->FFmpeg->WriteAudio(chunk.Data.data(), chunk.Data.size());
            m_TotalAudioSamplesWritten += (chunk.Data.size() / m_MasterFormat.BytesPerFrame);
        }
    }

    if (m_TotalAudioSamplesWritten < expectedSamples)
    {
        uint64_t missing = expectedSamples - m_TotalAudioSamplesWritten;
        if (missing > (m_MasterFormat.SamplesPerSec / 100))
        {
            if (missing > m_MasterFormat.SamplesPerSec) missing = m_MasterFormat.SamplesPerSec;

            std::vector<BYTE> silence(missing * m_MasterFormat.BytesPerFrame, 0);
            g_pSystem->Infrastructure->FFmpeg->WriteAudio(silence.data(), silence.size());
            m_TotalAudioSamplesWritten += missing;
        }
    }
}


void CaptureThread::Cleanup()
{
    m_TotalVideoFramesWritten.store(0);
    m_TotalAudioSamplesWritten.store(0);
    m_SyncTimeInitialized.store(false);
    m_SyncInitialized.store(false);
    m_MasterFormat = AudioFormat{};
    m_LastFrameBuffer.clear();
}