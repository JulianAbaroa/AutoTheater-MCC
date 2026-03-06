#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Threads/CaptureThread.h"

// TODO: It seems recording at 240 desynchronizes the video & audio.

using namespace std::chrono_literals;

void CaptureThread::Run()
{
    g_pUtil->Log.Append("[CaptureThread] INFO: Started.");

    while (g_pState->Lifecycle.IsRunning())
    {
        this->VerifyAndPrepareFFmpeg();
        if (!this->ReadyToCapture())
        {
            std::this_thread::sleep_for(10ms);
            continue;
        }

        if (g_pState->FFmpeg.RecordingStarted() && !g_pState->FFmpeg.IsRecording())
        {
            if (g_pState->Audio.GetMasterInstance() != nullptr)
            {
                this->StartRecording();
            }
            else
            {
                std::this_thread::sleep_for(1ms);
                continue;
            }
        }

        if (g_pState->FFmpeg.IsRecording())
        {
            if (this->VideoQueueOverflow()) continue;

            int currentFPS = g_pHook->TargetFPS.GetCurrentFPSValue();
            if (currentFPS == 0.0f)
            {
                g_pSystem->FFmpeg.ForceStop();
                g_pState->FFmpeg.SetTargetFramerate(static_cast<float>(currentFPS));
                g_pUtil->Log.Append("[CaptureThread] ERROR: AutoTheater cannot record with UNLIMITED framerate active, recording stopped.");
                continue;
            }

            int targetFPS = static_cast<int>(g_pState->FFmpeg.GetTargetFramerate());

            bool wasFramerateChanged = (currentFPS != targetFPS);
            if (g_pState->FFmpeg.RecordingStopped() || wasFramerateChanged)
            {
                if (wasFramerateChanged)
                {
                    g_pSystem->FFmpeg.ForceStop();
                    g_pState->FFmpeg.SetTargetFramerate(static_cast<float>(currentFPS));

                    g_pUtil->Log.Append("[CaptureThread] WARNING: Recording stopped, the game framerate"
                        " was changed from %d to %d FPS.", targetFPS, currentFPS);
                }
                else
                {
                    g_pSystem->FFmpeg.Stop();
                }

                g_pSystem->Gallery.RefreshList(g_pState->FFmpeg.GetOutputPath());
                continue;
            }

            if (!m_SyncInitialized)
            {
                if (g_pSystem->Video.GetQueueSize() > 0)
                {
                    auto audioQueue = g_pSystem->Audio.ExtractQueue();
                    auto videoQueue = g_pSystem->Video.ExtractQueue();

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
                auto audio = g_pSystem->Audio.ExtractQueue();
                auto video = g_pSystem->Video.ExtractQueue();

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

    if (g_pState->FFmpeg.IsRecording()) g_pSystem->FFmpeg.ForceStop();
    g_pUtil->Log.Append("[CaptureThread] INFO: Stopped.");
}

void CaptureThread::VerifyAndPrepareFFmpeg()
{
    if (g_pState->Settings.ShouldUseAppData() &&
        !g_pState->FFmpeg.IsFFmpegInstalled() &&
        !g_pState->FFmpeg.IsDownloadInProgress())
    {
        g_pSystem->FFmpeg.InitializeFFmpeg();
    }
}

bool CaptureThread::ReadyToCapture()
{
    return  g_pState->Theater.IsTheaterMode() &&
            g_pState->Director.IsInitialized() &&
            g_pState->Theater.GetTimePtr() != nullptr;
}

void CaptureThread::StartRecording()
{
    g_pState->FFmpeg.SetStartRecording(false);

    g_pSystem->Video.ExtractQueue();
    g_pSystem->Audio.ExtractQueue();
    m_LastFrameBuffer.clear();

    int currentFPS = g_pHook->TargetFPS.GetCurrentFPSValue();
    if (currentFPS == 0.0f)
    {
        g_pState->FFmpeg.SetTargetFramerate(static_cast<float>(currentFPS));
        g_pUtil->Log.Append("[CaptureThread] ERROR: AutoTheater cannot record with UNLIMITED framerate active, start recording cancelled.");
        return;
    }

    int targetFPS = static_cast<int>(g_pState->FFmpeg.GetTargetFramerate());

    if (currentFPS != targetFPS)
    {
        g_pState->FFmpeg.SetTargetFramerate(static_cast<float>(currentFPS));
        g_pUtil->Log.Append("[CaptureThread] WARNING: Detected framerate change from %d to %d,"
            " recording at %d FPS.", targetFPS, currentFPS, currentFPS);
    }

    bool started = g_pSystem->FFmpeg.Start(
        g_pState->FFmpeg.GetOutputPath(),
        g_pState->Render.GetWidth(), g_pState->Render.GetHeight(),
        static_cast<float>(currentFPS)
    );

    if (started)
    {
        g_pSystem->Video.StartRecording();
        g_pSystem->Audio.StartRecording();
        m_SyncTimeInitialized.store(false);
        m_SyncInitialized.store(false);
    }
    else
    {
        g_pUtil->Log.Append("[CaptureThread] ERROR: FFmpeg.Start() failed.");
    }
}

void CaptureThread::StopRecording()
{
    g_pState->FFmpeg.SetStopRecording(false);
    g_pSystem->FFmpeg.Stop();
}

bool CaptureThread::VideoQueueOverflow()
{
    if (g_pSystem->Video.GetQueueSize() <= 64) return false;

    if (!m_SyncInitialized)
    {
        g_pSystem->Video.ClearQueue();
        g_pSystem->Audio.ClearQueue();
        return true;
    }
    else
    {
        g_pUtil->Log.Append("[CaptureThread] ERROR: Frame queue overload, aborting recording.");
        g_pSystem->FFmpeg.ForceStop();
        return true;
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
    else if (!audioQueue.empty() && g_pState->Audio.GetMasterInstance() != nullptr)
    {
        ready = true;
        mode = "Audio Priority";
    }

    if (ready)
    {
        m_RecordingStartTime = std::chrono::steady_clock::now();

        g_pState->FFmpeg.SetStartRecordingTime(std::chrono::steady_clock::now());

        m_TotalVideoFramesWritten = 0;
        m_TotalAudioSamplesWritten = 0;
        m_SyncInitialized = true;

        if (!videoQueue.empty()) m_LastFrameBuffer = videoQueue.back().buffer;

        void* masterInstance = g_pState->Audio.GetMasterInstance();
        if (masterInstance != nullptr)
        {
            m_MasterFormat = g_pState->Audio.GetActiveInstance(masterInstance);
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
        m_RecordingStartTime = std::chrono::steady_clock::now();
        m_SyncTimeInitialized = true;
        g_pState->FFmpeg.SetCaptureActive(true);
    }

    auto now = std::chrono::steady_clock::now();
    float elapsedRealTime = std::chrono::duration<float>(now - m_RecordingStartTime).count();
    uint64_t expectedFrames = static_cast<uint64_t>(elapsedRealTime * g_pState->FFmpeg.GetTargetFramerate());
    uint64_t expectedSamples = static_cast<uint64_t>(elapsedRealTime * m_MasterFormat.SamplesPerSec);

    for (auto& frame : videoQueue)
    {
        if (m_TotalVideoFramesWritten <= expectedFrames)
        {
            g_pSystem->FFmpeg.WriteVideo(frame.buffer.data(), frame.buffer.size());
            m_TotalVideoFramesWritten++;
        }
    }

    for (auto& chunk : audioQueue)
    {
        if (chunk.data.size() > 0)
        {
            g_pSystem->FFmpeg.WriteAudio(chunk.data.data(), chunk.data.size());
            m_TotalAudioSamplesWritten += (chunk.data.size() / m_MasterFormat.BytesPerFrame);
        }
    }

    if (m_TotalAudioSamplesWritten < expectedSamples)
    {
        uint64_t missing = expectedSamples - m_TotalAudioSamplesWritten;
        if (missing > (m_MasterFormat.SamplesPerSec / 100))
        {
            if (missing > m_MasterFormat.SamplesPerSec) missing = m_MasterFormat.SamplesPerSec;

            std::vector<BYTE> silence(missing * m_MasterFormat.BytesPerFrame, 0);
            g_pSystem->FFmpeg.WriteAudio(silence.data(), silence.size());
            m_TotalAudioSamplesWritten += missing;
        }
    }
}

void CaptureThread::MonitorRecordingHealth()
{
    if (!g_pState->FFmpeg.IsRecording()) return;

    HANDLE hProcess = g_pState->FFmpeg.GetProcessHandle();
    if (hProcess == NULL) return;

    DWORD exitCode = 0;
    if (hProcess && GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE)
    {
        g_pUtil->Log.Append("[CaptureThread] ERROR: FFmpeg process died. Exit code: %lu. Restarting.", exitCode);
        this->StopRecording();
        std::this_thread::sleep_for(500ms);
        g_pState->FFmpeg.SetStartRecording(true);
        g_pUtil->Log.Append("[CaptureThread] WARNING: Recovery triggered, starting new segment.");
    }
}