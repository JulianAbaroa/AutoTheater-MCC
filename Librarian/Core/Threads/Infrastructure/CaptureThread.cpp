#include "pch.h"
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
#include "Core/States/Infrastructure/Capture/DownloadState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"
#include "Core/Systems/Infrastructure/Capture/AudioSystem.h"
#include "Core/Systems/Infrastructure/Capture/SyncSystem.h"
#include "Core/Systems/Infrastructure/Persistence/GallerySystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Threads/Infrastructure/CaptureThread.h"

using namespace std::chrono_literals;

void CaptureThread::Run()
{
    g_pSystem->Debug->Log("[CaptureThread] INFO: Started.");

    std::deque<AudioChunk> pendingAudio;
    std::deque<FrameData> pendingVideo;

    g_pSystem->Infrastructure->Gallery->RefreshList(g_pState->Infrastructure->FFmpeg->GetOutputPath());

    while (g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        this->VerifyAndPrepareFFmpeg();
        if (!this->ReadyToCapture()) 
        {
            std::this_thread::sleep_for(10ms);
            continue;
        }

        if (g_pState->Infrastructure->FFmpeg->RecordingStarted() && !g_pState->Infrastructure->FFmpeg->IsRecording()) 
        {
            this->StartRecording();
        }

        if (g_pState->Infrastructure->FFmpeg->IsRecording())
        {
            if (g_pSystem->Infrastructure->FFmpeg->HasFatalError())
            {
                g_pSystem->Debug->Log("[CaptureThread] ERROR: FFmpeg reported fatal error,"
                    " forcing stop.");
                this->ProcessSynchronizedStreams(pendingAudio, pendingVideo, true);
                g_pSystem->Infrastructure->FFmpeg->ForceStop();
                this->Cleanup();
                pendingAudio.clear();
                pendingVideo.clear();
                continue;
            }

            g_pSystem->Infrastructure->Audio->Update();

            auto extAudio = g_pSystem->Infrastructure->Audio->ExtractQueue();
            auto extVideo = g_pSystem->Infrastructure->Video->ExtractQueue();

            if (!extAudio.empty())
            {
                pendingAudio.insert(pendingAudio.end(),
                    std::make_move_iterator(extAudio.begin()),
                    std::make_move_iterator(extAudio.end()));
            }
            if (!extVideo.empty())
            {
                pendingVideo.insert(pendingVideo.end(), std::make_move_iterator(extVideo.begin()), std::make_move_iterator(extVideo.end()));
            }

            if (this->VideoQueueOverflow(pendingVideo.size()))
            {
                g_pSystem->Infrastructure->Video->ReturnBuffer(std::move(pendingVideo.front().Buffer));
                pendingVideo.pop_front();

                g_pSystem->Debug->Log("[CaptureThread] Video queue reduced to %zu frames.", pendingVideo.size());
            }

            int currentFramerate = g_pHook->Memory->TargetFramerate->GetCurrentFramerateValue();
            int targetFramerate = static_cast<int>(g_pState->Infrastructure->FFmpeg->GetTargetFramerate());

            bool wasFramerateChanged = (currentFramerate != targetFramerate && currentFramerate != 0);

            if (g_pState->Infrastructure->FFmpeg->RecordingStopped() || wasFramerateChanged)
            {
                this->ProcessSynchronizedStreams(pendingAudio, pendingVideo, true);

                if (wasFramerateChanged) 
                {
                    g_pSystem->Infrastructure->FFmpeg->ForceStop();
                    g_pState->Infrastructure->FFmpeg->SetTargetFramerate(static_cast<float>(currentFramerate));
                }
                else 
                {
                    g_pSystem->Infrastructure->FFmpeg->Stop();
                }

                this->Cleanup();
                pendingAudio.clear();
                pendingVideo.clear();

                g_pSystem->Infrastructure->Gallery->RefreshList(g_pState->Infrastructure->FFmpeg->GetOutputPath());

                continue;
            }

            if (!m_SyncInitialized) 
            {
                if (this->CaptureBaselineEstablished(pendingAudio, pendingVideo)) 
                {
                    this->ProcessSynchronizedStreams(pendingAudio, pendingVideo, false);
                }

                std::this_thread::sleep_for(1ms);
                continue;
            }

            if (pendingAudio.empty() && pendingVideo.empty())
            {
                std::this_thread::sleep_for(1ms);
            }
            else
            {
                this->ProcessSynchronizedStreams(pendingAudio, pendingVideo, false);

                auto wait = g_pSystem->Infrastructure->Sync->TimeUntilNextFrame();
                if (wait.count() > 0)
                {
                    std::this_thread::sleep_for(wait);
                }
            }
        }
        else 
        {
            std::this_thread::sleep_for(10ms);
        }
    }
}


double CaptureThread::GetSyncRatio() const
{
    return m_SyncRatio.load();
}


void CaptureThread::VerifyAndPrepareFFmpeg()
{
    if (g_pState->Infrastructure->Settings->ShouldUseAppData() &&
        !g_pState->Infrastructure->Download->IsFFmpegInstalled() &&
        !g_pState->Infrastructure->Download->IsDownloadInProgress())
    {
        g_pSystem->Infrastructure->FFmpeg->InitializeDependencies();
    }
}

bool CaptureThread::ReadyToCapture()
{
    return  g_pState->Domain->Theater->IsTheaterMode() &&
            g_pState->Domain->Theater->GetTimePtr() != nullptr;
}


void CaptureThread::StartRecording()
{
    g_pSystem->Debug->Log("[CaptureThread] INFO: StartRecording invoked. RecordingStartedFlag will be cleared.");
    g_pState->Infrastructure->FFmpeg->SetStartRecording(false);

    int currentFramerate = g_pHook->Memory->TargetFramerate->GetCurrentFramerateValue();
    if (currentFramerate == 0)
    {
        g_pState->Infrastructure->FFmpeg->SetTargetFramerate(static_cast<float>(currentFramerate));
        g_pSystem->Debug->Log("[CaptureThread] ERROR: AutoTheater cannot record with UNLIMITED framerate active (0). Start canceled.");
        return;
    }

    int targetFramerate = static_cast<int>(g_pState->Infrastructure->FFmpeg->GetTargetFramerate());
    if (currentFramerate != targetFramerate)
    {
        g_pState->Infrastructure->FFmpeg->SetTargetFramerate(static_cast<float>(currentFramerate));
        g_pSystem->Debug->Log("[CaptureThread] INFO: Detected framerate change from %d to %d, recording at %d FPS.", targetFramerate, currentFramerate, currentFramerate);
    }

    bool isStarted = g_pSystem->Infrastructure->FFmpeg->Start(
        g_pState->Infrastructure->FFmpeg->GetOutputPath(),
        g_pState->Infrastructure->Render->GetWidth(),
        g_pState->Infrastructure->Render->GetHeight(),
        static_cast<float>(currentFramerate)
    );

    if (isStarted)
    {
        g_pSystem->Debug->Log("[CaptureThread] INFO: FFmpeg Start returned true. Performing post-start cleanup and enabling video/audio recording.");

        this->Cleanup();
        g_pSystem->Infrastructure->Video->StartRecording();
        g_pSystem->Infrastructure->Audio->StartRecording();
    }
    else
    {
        g_pSystem->Debug->Log("[CaptureThread] ERROR: Start recording failed (FFmpegSystem::Start returned false).");
    }
}

void CaptureThread::StopRecording() 
{
    g_pState->Infrastructure->FFmpeg->SetStopRecording(false);
    g_pSystem->Debug->Log("[CaptureThread] INFO: Recording stopped.");
}


bool CaptureThread::VideoQueueOverflow(size_t pendingSize)
{
    if (pendingSize <= 64) return false;

    if (!m_SyncInitialized)
    {
        g_pSystem->Infrastructure->Video->ClearQueue();
        g_pSystem->Infrastructure->Audio->ClearQueue();
        return true;
    }
    else
    {
        g_pSystem->Debug->Log("[CaptureThread] WARNING: Frame queue overload (%zu frames), dropping frames.", pendingSize);
        return true;
    }
}


bool CaptureThread::CaptureBaselineEstablished(std::deque<AudioChunk>& audioQueue, std::deque<FrameData>& videoQueue)
{
    if (audioQueue.empty() || videoQueue.empty()) return false;

    m_TotalVideoFramesWritten = 0;
    m_TotalAudioSamplesWritten = 0;

    int targetFramerate = static_cast<int>(g_pState->Infrastructure->FFmpeg->GetTargetFramerate());
    g_pSystem->Infrastructure->Sync->InitializeBaseline(targetFramerate);

    /*auto now = std::chrono::steady_clock::now();
    g_pState->Infrastructure->FFmpeg->SetStartRecordingTime(now);*/

    m_SyncInitialized = true;
    g_pState->Infrastructure->FFmpeg->SetCaptureActive(true);

    g_pSystem->Debug->Log("[CaptureThread] INFO: Sync Established.");

    return true;
}

void CaptureThread::ProcessSynchronizedStreams(
    std::deque<AudioChunk>& audioQueue,
    std::deque<FrameData>& videoQueue,
    bool forceDrain)
{
    int processedInThisCycle = 0;

    while ((!audioQueue.empty() || !videoQueue.empty()) && processedInThisCycle < m_MaxItemsPerCycle)
    {
        processedInThisCycle++;

        SyncDecision decision = g_pSystem->Infrastructure->Sync->DecideNext(
            videoQueue.empty() ? nullptr : &videoQueue.front(),
            audioQueue.empty() ? nullptr : &audioQueue.front());

        if (decision == SyncDecision::None) break;

        switch (decision) 
        {
        case SyncDecision::WriteAudio: 
        {
            AudioChunk chunk = std::move(audioQueue.front());
            audioQueue.pop_front();

            double durationSec = chunk.DurationSec;

            if (g_pSystem->Infrastructure->FFmpeg->WriteAudio(chunk.Data.data(), chunk.Data.size()))
            {
                g_pSystem->Infrastructure->Sync->OnAudioWritten(durationSec);
                m_TotalAudioSamplesWritten++;
                m_TotalAudioDuration += durationSec;
            }

            break;
        }

        case SyncDecision::WriteVideo: 
        {
            FrameData frame = std::move(videoQueue.front());
            videoQueue.pop_front();

            if (frame.Buffer.empty()) 
            {
                g_pSystem->Infrastructure->Video->ReturnBuffer(std::move(frame.Buffer));
                break;
            }

            m_LastFrameBuffer = frame.Buffer;

            if (g_pSystem->Infrastructure->FFmpeg->WriteVideo(frame.Buffer.data(), frame.Buffer.size())) 
            {
                g_pSystem->Infrastructure->Sync->OnVideoWritten();
                m_TotalVideoFramesWritten++;
            }

            g_pSystem->Infrastructure->Video->ReturnBuffer(std::move(frame.Buffer));
            break;
        }

        case SyncDecision::RepeatVideo: 
        {
            if (!m_LastFrameBuffer.empty()) 
            {
                if (g_pSystem->Infrastructure->FFmpeg->WriteVideo(m_LastFrameBuffer.data(), m_LastFrameBuffer.size())) 
                {
                    g_pSystem->Infrastructure->Sync->OnVideoWritten();
                    m_TotalVideoFramesWritten++;
                }
            }

            break;
        }

        case SyncDecision::DropVideo: 
        {
            FrameData frame = std::move(videoQueue.front());
            videoQueue.pop_front();

            g_pSystem->Infrastructure->Video->ReturnBuffer(std::move(frame.Buffer));
            break;
        }
        }
    }

    double expectedFrames = m_TotalAudioDuration * g_pState->Infrastructure->FFmpeg->GetTargetFramerate();
    m_SyncRatio.store((expectedFrames > 0) ? (m_TotalVideoFramesWritten / expectedFrames) : 1.0);

    if (forceDrain)
    {
        g_pSystem->Debug->Log("[CaptureThread] Drain: flushing %zu video"
            " frames and %zu audio chunks", videoQueue.size(), audioQueue.size());

        int drainedFrames = 0;
        while (!videoQueue.empty())
        {
            FrameData frame = std::move(videoQueue.front());
            videoQueue.pop_front();
            g_pSystem->Infrastructure->FFmpeg->WriteVideo(frame.Buffer.data(), frame.Buffer.size());
            g_pSystem->Infrastructure->Video->ReturnBuffer(std::move(frame.Buffer));
            drainedFrames++;
        }
        audioQueue.clear();

        g_pSystem->Debug->Log("[CaptureThread] INFO: completed, %d frames written", drainedFrames);
    }
}

void CaptureThread::Cleanup()
{
    m_LastFrameBuffer.clear();

    m_SyncInitialized.store(false);
    m_SyncTimeInitialized.store(false);

    m_LastStatLog = {};
    m_TotalVideoFramesWritten = 0;
    m_TotalAudioSamplesWritten = 0;
    m_TotalAudioDuration = 0.0;
    m_SyncRatio = 0.0;

    g_pSystem->Debug->Log("[CaptureThread] INFO: cleanup completed.");
}