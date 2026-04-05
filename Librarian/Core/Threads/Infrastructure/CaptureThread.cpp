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
#include "Core/States/Infrastructure/Capture/ProcessState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/States/Infrastructure/Capture/VideoState.h"
#include "Core/States/Infrastructure/Capture/DownloadState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Infrastructure/Capture/ProcessSystem.h"
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

    g_pSystem->Infrastructure->Gallery->RefreshList(
        g_pState->Infrastructure->FFmpeg->GetOutputPath());

    while (g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        this->VerifyFFmpeg();

        if (!this->IsReady()) 
        {
            std::this_thread::sleep_for(15ms);
            continue;
        }

        if (g_pState->Infrastructure->FFmpeg->RecordingStarted() && 
            !g_pState->Infrastructure->FFmpeg->IsRecording()) 
        {
            this->StartRecording();
        }

        if (g_pState->Infrastructure->FFmpeg->IsRecording())
        {
            if (g_pState->Infrastructure->Process->HasFatalError())
            {
                if (!m_StopInProgress.exchange(true))
                {
				    this->StopRecording(true);
                }

                continue;
            }

            g_pSystem->Infrastructure->Audio->Update();

            auto audioQueue = g_pSystem->Infrastructure->Audio->ExtractQueue();
            auto videoQueue = g_pSystem->Infrastructure->Video->ExtractQueue();

            if (!audioQueue.empty())
            {
                m_PendingAudio.insert(m_PendingAudio.end(),
                    std::make_move_iterator(audioQueue.begin()),
                    std::make_move_iterator(audioQueue.end()));
            }

            if (!videoQueue.empty())
            {
                m_PendingVideo.insert(m_PendingVideo.end(), 
                    std::make_move_iterator(videoQueue.begin()), 
                    std::make_move_iterator(videoQueue.end()));
            }

            if (this->VideoQueueOverflow(m_PendingVideo.size()))
            {
                g_pSystem->Infrastructure->Video->ReturnBuffer(
                    std::move(m_PendingVideo.front().Buffer));

                m_PendingVideo.pop_front();

                g_pSystem->Debug->Log("[CaptureThread] Video queue reduced to"
                    " %zu frames.", m_PendingVideo.size());
            }

            int currentFramerate = g_pHook->Memory->TargetFramerate->GetCurrentFramerateValue();
            int targetFramerate = static_cast<int>(g_pState->Infrastructure->FFmpeg->GetTargetFramerate());

            bool recordingStopped = g_pState->Infrastructure->FFmpeg->RecordingStopped();
            bool wasFramerateChanged = (currentFramerate != targetFramerate && currentFramerate != 0);

            if ((recordingStopped || wasFramerateChanged) && !m_StopInProgress.exchange(true))
            {
                // We stop getting data.
                g_pState->Infrastructure->Audio->SetRecording(false);
				g_pState->Infrastructure->Video->SetRecording(false);

                // We force the draining of all pending data.
                this->ProcessSynchronizedStreams(true);

                // We flush and stop the writer thread.
				g_pThread->Writer->Flush();
                g_pThread->Writer->StopRecording();

                this->Cleanup();

                // We stop and clear the audio and video systems.
                g_pSystem->Infrastructure->Audio->StopRecording();
                g_pSystem->Infrastructure->Video->StopRecording();

                if (wasFramerateChanged || m_StopByForce.load())
                {
                    m_StopByForce.store(false);

                    g_pSystem->Infrastructure->FFmpeg->ForceStop();

                    if (wasFramerateChanged)
                    {
                        g_pState->Infrastructure->FFmpeg->SetTargetFramerate(
                            static_cast<float>(currentFramerate));
                    }
                }
                else 
                {
                    g_pSystem->Infrastructure->FFmpeg->Stop();
                }

                g_pSystem->Infrastructure->Gallery->RefreshList(
                    g_pState->Infrastructure->FFmpeg->GetOutputPath());
                
                m_StopInProgress.store(false);
                continue;
            }

            if (!m_SyncInitialized) 
            {
                if (this->CaptureBaselineEstablished())
                {
                    this->ProcessSynchronizedStreams(false);
                }

                continue;
            }

			if (!m_PendingAudio.empty() || !m_PendingVideo.empty())
            {
                this->ProcessSynchronizedStreams(false);

                auto wait = g_pSystem->Infrastructure->Sync->TimeUntilNextFrame();
                if (wait.count() > 0)
                {
                    std::this_thread::sleep_for(wait);
                }
            }
        }
        else 
        {
            std::this_thread::sleep_for(15ms);
        }
    }
}


void CaptureThread::StartRecording()
{
    g_pState->Infrastructure->FFmpeg->SetStartRecording(false);

    int currentFramerate = g_pHook->Memory->TargetFramerate->GetCurrentFramerateValue();
    if (!this->CheckFramerate(currentFramerate)) return;

    std::string outputPath = g_pState->Infrastructure->FFmpeg->GetOutputPath();
    int width = g_pState->Infrastructure->Render->GetWidth();
    int height = g_pState->Infrastructure->Render->GetHeight();

    bool isStarted = g_pSystem->Infrastructure->FFmpeg->Start(
        outputPath, width, height, static_cast<float>(currentFramerate));

    if (isStarted)
    {
        auto configuration = g_pState->Infrastructure->FFmpeg->GetConfiguration();

        m_CaptureMaxFrames.store(configuration.MaxBufferedFrames / 2);
        m_WriterMaxFrames.store(configuration.MaxBufferedFrames / 4);

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
    m_StopByForce.store(force);
    g_pState->Infrastructure->FFmpeg->SetStopRecording(true);
    g_pSystem->Debug->Log("[CaptureThread] INFO: Recording stopped.");
}


void CaptureThread::VerifyFFmpeg()
{
	bool shouldUseAppData = g_pState->Infrastructure->Settings->ShouldUseAppData();
	bool isFFmpegInstalled = g_pState->Infrastructure->Download->IsFFmpegInstalled();
	bool isDownloadInProgress = g_pState->Infrastructure->Download->IsDownloadInProgress();

    if (shouldUseAppData && !isFFmpegInstalled && !isDownloadInProgress)
    {
        g_pSystem->Infrastructure->Process->InitializeDependencies();
    }
}

bool CaptureThread::IsReady()
{
	bool isTheaterMode = g_pState->Domain->Theater->IsTheaterMode();
	float* pTime = g_pState->Domain->Theater->GetTimePtr();

    return isTheaterMode && pTime != nullptr;
}


bool CaptureThread::CheckFramerate(int framerate)
{
    if (framerate == 0)
    {
        g_pState->Infrastructure->FFmpeg->SetTargetFramerate(static_cast<float>(framerate));

        g_pSystem->Debug->Log("[CaptureThread] ERROR: AutoTheater cannot record with UNLIMITED"
            " framerate active (0). Start canceled.");

        return false;
    }

    float targetFramerate = g_pState->Infrastructure->FFmpeg->GetTargetFramerate();
    if (framerate != static_cast<int>(targetFramerate))
    {
        g_pState->Infrastructure->FFmpeg->SetTargetFramerate(static_cast<float>(framerate));

        g_pSystem->Debug->Log("[CaptureThread] INFO: Detected framerate change from %d to %d,"
            " recording at %d FPS.", static_cast<int>(targetFramerate), framerate, framerate);
    }

    return true;
}


bool CaptureThread::VideoQueueOverflow(size_t pendingSize)
{
    if (pendingSize <= m_CaptureMaxFrames.load()) return false;

    if (!m_SyncInitialized)
    {
        g_pSystem->Infrastructure->Video->ClearQueue();
        g_pSystem->Infrastructure->Audio->ClearQueue();

        return true;
    }
    else
    {
        g_pSystem->Debug->Log("[CaptureThread] WARNING: Frame queue overload (%zu frames),"
            " dropping frames.", pendingSize);

        return true;
    }
}


bool CaptureThread::CaptureBaselineEstablished()
{
    if (m_PendingAudio.empty() || m_PendingVideo.empty()) return false;

    this->ClearPendingResources();

    auto now = std::chrono::steady_clock::now();
    g_pState->Infrastructure->FFmpeg->SetStartRecordingTime(now);

    float targetFramerate = g_pState->Infrastructure->FFmpeg->GetTargetFramerate();
    g_pSystem->Infrastructure->Sync->InitializeBaseline(static_cast<int>(targetFramerate));

    m_SyncInitialized = true;
    g_pState->Infrastructure->FFmpeg->SetCaptureActive(true);

    g_pSystem->Debug->Log("[CaptureThread] INFO: Sync Established.");
    return true;
}

void CaptureThread::ClearPendingResources()
{
    m_VideoFramesWritten = 0;

    while (!m_PendingVideo.empty())
    {
        g_pSystem->Infrastructure->Video->ReturnBuffer(
            std::move(m_PendingVideo.front().Buffer));

        m_PendingVideo.pop_front();
    }

    m_PendingAudio.clear();

    g_pSystem->Infrastructure->Audio->FlushPendingSamples();
}

void CaptureThread::ProcessSynchronizedStreams(bool forceDrain)
{
    int processedInThisCycle = 0;
    int itemsThisCycle = (std::max)(60, (int)(m_PendingVideo.size() + m_PendingAudio.size()));

    while ((!m_PendingAudio.empty() || !m_PendingVideo.empty()) && processedInThisCycle < itemsThisCycle)
    {
        processedInThisCycle++;

        SyncDecision decision = g_pSystem->Infrastructure->Sync->DecideNext(
            m_PendingVideo.empty() ? nullptr : &m_PendingVideo.front(),
            m_PendingAudio.empty() ? nullptr : &m_PendingAudio.front());

        if (decision == SyncDecision::None) break;

        switch (decision)
        {
        case SyncDecision::WriteAudio:
        {
            AudioChunk chunk = std::move(m_PendingAudio.front());
            m_PendingAudio.pop_front();

            double durationSec = chunk.DurationSec;

            g_pThread->Writer->EnqueueAudio(std::move(chunk.Data));

            g_pSystem->Infrastructure->Sync->OnAudioWritten(durationSec);
            m_AudioDuration += durationSec;
            break;
        }

        case SyncDecision::WriteVideo:
        {
            FrameData frame = std::move(m_PendingVideo.front());
            m_PendingVideo.pop_front();

            if (frame.Buffer.empty()) break;

            std::vector<uint8_t> copyBuffer = g_pSystem->Infrastructure->Video->GetFreeBuffer();

            if (!copyBuffer.empty())
            {
                copyBuffer.assign(frame.Buffer.begin(), frame.Buffer.end());

                if (!m_LastFrameBuffer.empty())
                    g_pSystem->Infrastructure->Video->ReturnBuffer(std::move(m_LastFrameBuffer));

                m_LastFrameBuffer = std::move(copyBuffer);
            }
            else
            {
                g_pSystem->Debug->Log("[CaptureThread] WARNING: Pool exhausted,"
                    " LastFrame not updated.");
            }

            g_pThread->Writer->EnqueueVideo(std::move(frame.Buffer), true);

            g_pSystem->Infrastructure->Sync->OnVideoWritten();
            m_VideoFramesWritten++;
            break;
        }

        case SyncDecision::RepeatVideo:
        {
            if (!m_LastFrameBuffer.empty())
            {
                std::vector<uint8_t> repeatBuffer = g_pSystem->Infrastructure->Video->GetFreeBuffer();

                repeatBuffer.assign(m_LastFrameBuffer.begin(), m_LastFrameBuffer.end());

                g_pThread->Writer->EnqueueVideo(std::move(repeatBuffer), true);

                g_pSystem->Infrastructure->Sync->OnVideoWritten();
                m_VideoFramesWritten++;
            }

            break;
        }

        case SyncDecision::DropVideo:
        {
            FrameData frame = std::move(m_PendingVideo.front());
            m_PendingVideo.pop_front();

            g_pSystem->Infrastructure->Video->ReturnBuffer(std::move(frame.Buffer));
            break;
        }
        }
    }

    double expectedFrames = m_AudioDuration * g_pState->Infrastructure->FFmpeg->GetTargetFramerate();
    m_SyncRatio = (expectedFrames > 0) ? (m_VideoFramesWritten / expectedFrames) : 1.0;

    if (forceDrain)
    {
        while (!m_PendingVideo.empty())
        {
            FrameData frame = std::move(m_PendingVideo.front());
            m_PendingVideo.pop_front();

            if (!frame.Buffer.empty())
            {
                g_pThread->Writer->EnqueueVideo(std::move(frame.Buffer), true);
            }
        }

        while (!m_PendingAudio.empty())
        {
            AudioChunk chunk = std::move(m_PendingAudio.front());
            m_PendingAudio.pop_front();
            g_pThread->Writer->EnqueueAudio(std::move(chunk.Data));
        }
    }
}


void CaptureThread::Cleanup()
{
	size_t cachedBufferSize = g_pState->Infrastructure->Video->GetFrameByteSize();
    if (m_LastFrameBuffer.size() == cachedBufferSize)
    {
        g_pSystem->Infrastructure->Video->ReturnBuffer(
            std::move(m_LastFrameBuffer));
    }
    else
    {
        m_LastFrameBuffer.clear();
    }

    m_SyncInitialized = false;
    m_StopByForce.store(false);
    m_SyncRatio = 0.0;

    while (!m_PendingVideo.empty())
    {
        if (!m_PendingVideo.front().Buffer.empty())
        {
            g_pSystem->Infrastructure->Video->ReturnBuffer(
                std::move(m_PendingVideo.front().Buffer));
        }
        m_PendingVideo.pop_front();
    }

    m_PendingAudio = {};

    m_VideoFramesWritten = 0;
    m_AudioDuration = 0.0;

    m_CaptureMaxFrames.store(0);
    m_WriterMaxFrames.store(0);

    g_pSystem->Debug->Log("[CaptureThread] INFO: cleanup completed.");
}