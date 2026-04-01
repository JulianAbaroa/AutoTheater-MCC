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
#include "Core/States/Infrastructure/Capture/VideoState.h"
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
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Infrastructure/CaptureThread.h"
#include "Core/Threads/Infrastructure/WriterThread.h"

using namespace std::chrono_literals;

void CaptureThread::Run()
{
    g_pSystem->Debug->Log("[CaptureThread] INFO: Started.");

    g_pSystem->Infrastructure->Gallery->RefreshList(g_pState->Infrastructure->FFmpeg->GetOutputPath());

    m_LastStatLog = std::chrono::steady_clock::now();

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
                g_pSystem->Debug->Log("[CaptureThread] ERROR: FFmpeg reported" 
                    " fatal error, forcing stop.");

                this->ProcessSynchronizedStreams(m_PendingAudio, m_PendingVideo, true);
                m_PendingAudio.clear();
                m_PendingVideo.clear();

                g_pSystem->Infrastructure->Audio->StopRecording();
                g_pSystem->Infrastructure->Video->StopRecording();
                g_pSystem->Infrastructure->FFmpeg->ForceStop();

                this->Cleanup();
                g_pSystem->Infrastructure->Gallery->RefreshList(g_pState->Infrastructure->FFmpeg->GetOutputPath());
                continue;
            }

            g_pSystem->Infrastructure->Audio->Update();

            auto extAudio = g_pSystem->Infrastructure->Audio->ExtractQueue();
            auto extVideo = g_pSystem->Infrastructure->Video->ExtractQueue();

            if (!extAudio.empty())
            {
                m_PendingAudio.insert(m_PendingAudio.end(),
                    std::make_move_iterator(extAudio.begin()),
                    std::make_move_iterator(extAudio.end()));
            }
            if (!extVideo.empty())
            {
                m_PendingVideo.insert(m_PendingVideo.end(), std::make_move_iterator(extVideo.begin()), std::make_move_iterator(extVideo.end()));
            }

            if (this->VideoQueueOverflow(m_PendingVideo.size()))
            {
                g_pSystem->Infrastructure->Video->ReturnBuffer(std::move(m_PendingVideo.front().Buffer));
                m_PendingVideo.pop_front();

                g_pSystem->Debug->Log("[CaptureThread] Video queue reduced to %zu frames.", m_PendingVideo.size());
            }

            int currentFramerate = g_pHook->Memory->TargetFramerate->GetCurrentFramerateValue();
            int targetFramerate = static_cast<int>(g_pState->Infrastructure->FFmpeg->GetTargetFramerate());

            bool wasFramerateChanged = (currentFramerate != targetFramerate && currentFramerate != 0);

            if (g_pState->Infrastructure->FFmpeg->RecordingStopped() || wasFramerateChanged)
            {
                this->ProcessSynchronizedStreams(m_PendingAudio, m_PendingVideo, true);
                m_PendingAudio.clear();
                m_PendingVideo.clear();

                g_pSystem->Infrastructure->Audio->StopRecording();
                g_pSystem->Infrastructure->Video->StopRecording();

                if (wasFramerateChanged || m_StopByForce.load())
                {
                    m_StopByForce.store(false);
                    g_pSystem->Infrastructure->FFmpeg->ForceStop();
                    g_pState->Infrastructure->FFmpeg->SetTargetFramerate(static_cast<float>(currentFramerate));
                }
                else 
                {
                    g_pSystem->Infrastructure->FFmpeg->Stop();
                }

                this->Cleanup();
                g_pSystem->Infrastructure->Gallery->RefreshList(g_pState->Infrastructure->FFmpeg->GetOutputPath());
                continue;
            }

            if (!m_SyncInitialized) 
            {
                if (this->CaptureBaselineEstablished(m_PendingAudio, m_PendingVideo))
                {
                    this->ProcessSynchronizedStreams(m_PendingAudio, m_PendingVideo, false);
                }

                std::this_thread::sleep_for(1ms);
                continue;
            }

            if (m_PendingAudio.empty() && m_PendingVideo.empty())
            {
                std::this_thread::sleep_for(1ms);
            }
            else
            {
                auto nowPool = std::chrono::steady_clock::now();
                if (std::chrono::duration<double>(nowPool - m_LastStatLog).count() >= 5.0)
                {
                    uint64_t taken = g_pSystem->Infrastructure->Video->GetPoolTaken();
                    uint64_t returned = g_pSystem->Infrastructure->Video->GetPoolReturned();
                    uint64_t discarded = g_pSystem->Infrastructure->Video->GetPoolDiscarded();

                    g_pSystem->Debug->Log("[CaptureThread] DEBUG: Pool=%d/%d Taken=%llu Returned=%llu Discarded=%llu Balance=%lld PendingVideo=%zu PendingAudio=%zu",
                        g_pSystem->Infrastructure->Video->GetPoolSize(),
                        g_pSystem->Infrastructure->Video->GetPoolMaxSize(),
                        taken, returned, discarded,
                        (int64_t)taken - (int64_t)returned - (int64_t)discarded,
                        m_PendingVideo.size(),
                        m_PendingAudio.size());

                    m_LastStatLog = nowPool;
                }

                this->ProcessSynchronizedStreams(m_PendingAudio, m_PendingVideo, false);

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
    g_pState->Infrastructure->FFmpeg->SetStartRecording(false);

    // Update framerate.
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
        static_cast<float>(currentFramerate));

    if (isStarted)
    {
        auto config = g_pState->Infrastructure->FFmpeg->GetEncoderConfig();
        m_MaxFrames = config.MaxBufferedFrames / 2;
        m_WriterMaxFrames = config.MaxBufferedFrames / 4;
        g_pThread->Writer->StartRecording();
        g_pSystem->Infrastructure->Video->StartRecording();
        g_pSystem->Infrastructure->Audio->StartRecording();
        g_pSystem->Debug->Log("[CaptureThread] INFO: Recording started.");
    }
    else
    {
        g_pSystem->Debug->Log("[CaptureThread] ERROR: Start recording failed.");
    }
}

void CaptureThread::StopRecording(bool force) 
{
    m_PendingAudio = {};
    m_PendingVideo = {};
    m_StopByForce.store(force);
    g_pThread->Writer->StopRecording();
    g_pState->Infrastructure->Audio->SetRecording(false);
    g_pState->Infrastructure->Video->SetRecording(false);
    g_pState->Infrastructure->FFmpeg->SetStopRecording(true);
    g_pSystem->Debug->Log("[CaptureThread] INFO: Recording stopped.");
}


size_t CaptureThread::GetPendingAudioSize()
{
    std::lock_guard<std::mutex> lock(m_PendingMutex);
    return m_PendingAudio.size();
}

size_t CaptureThread::GetPendingVideoSize()
{
    std::lock_guard<std::mutex> lock(m_PendingMutex);
    return m_PendingVideo.size();
}


bool CaptureThread::VideoQueueOverflow(size_t pendingSize)
{
    if (pendingSize <= m_MaxFrames) return false;

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

    // Cleaning.
    while (!videoQueue.empty())
    {
        g_pSystem->Infrastructure->Video->ReturnBuffer(
            std::move(videoQueue.front().Buffer));
        videoQueue.pop_front();
    }
    audioQueue.clear();
    g_pSystem->Infrastructure->Audio->FlushPendingSamples();

    auto now = std::chrono::steady_clock::now();
    g_pState->Infrastructure->FFmpeg->SetStartRecordingTime(now);

    g_pSystem->Infrastructure->Sync->InitializeBaseline(targetFramerate);

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
    int itemsThisCycle = (std::max)(60, (int)(m_PendingVideo.size() + m_PendingAudio.size()));

    while ((!audioQueue.empty() || !videoQueue.empty()) && processedInThisCycle < itemsThisCycle)
    {
        processedInThisCycle++;

        SyncDecision decision = g_pSystem->Infrastructure->Sync->DecideNext(
            videoQueue.empty() ? nullptr : &videoQueue.front(),
            audioQueue.empty() ? nullptr : &audioQueue.front());

        if (decision == SyncDecision::None)
            break;

        switch (decision)
        {
        case SyncDecision::WriteAudio:
        {
            AudioChunk chunk = std::move(audioQueue.front());
            audioQueue.pop_front();

            double durationSec = chunk.DurationSec;

            g_pThread->Writer->EnqueueAudio(std::move(chunk.Data));

            g_pSystem->Infrastructure->Sync->OnAudioWritten(durationSec);
            m_TotalAudioSamplesWritten++;
            m_TotalAudioDuration += durationSec;
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

            if (!m_LastFrameBuffer.empty())
            {
                m_LastFrameBuffer.clear();
            }
            m_LastFrameBuffer = frame.Buffer;

            if (g_pThread->Writer->GetPendingSize() >= m_WriterMaxFrames)
            {
                g_pThread->Writer->DropOldestVideo();
            }
            else
            {
                g_pThread->Writer->EnqueueVideo(std::move(frame.Buffer), true);
            }

            g_pSystem->Infrastructure->Sync->OnVideoWritten();
            m_TotalVideoFramesWritten++;
            break;
        }

        case SyncDecision::RepeatVideo:
        {
            if (!m_LastFrameBuffer.empty())
            {
                std::vector<uint8_t> repeatCopy = m_LastFrameBuffer;
                g_pThread->Writer->EnqueueVideo(std::move(repeatCopy), false);

                g_pSystem->Infrastructure->Sync->OnVideoWritten();
                m_TotalVideoFramesWritten++;
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
        while (!videoQueue.empty())
        {
            FrameData frame = std::move(videoQueue.front());
            videoQueue.pop_front();

            if (!frame.Buffer.empty())
            {
                g_pThread->Writer->EnqueueVideo(std::move(frame.Buffer), true);
            }
        }

        while (!audioQueue.empty())
        {
            AudioChunk chunk = std::move(audioQueue.front());
            audioQueue.pop_front();
            g_pThread->Writer->EnqueueAudio(std::move(chunk.Data));
        }
    }
}

void CaptureThread::Cleanup()
{
    m_LastFrameBuffer.clear();

    m_SyncInitialized.store(false);
    m_SyncTimeInitialized.store(false);
    m_StopByForce.store(false);

    m_PendingAudio = {};
    m_PendingVideo = {};

    m_LastStatLog = {};
    m_TotalVideoFramesWritten = 0;
    m_TotalAudioSamplesWritten = 0;
    m_TotalAudioDuration = 0.0;
    m_SyncRatio = 0.0;

    g_pSystem->Debug->Log("[CaptureThread] INFO: cleanup completed.");
}