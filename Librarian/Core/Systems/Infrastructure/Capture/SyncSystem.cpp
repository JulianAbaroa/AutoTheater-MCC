#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/SyncSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

void SyncSystem::InitializeBaseline(double targetFramerate) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_TargetFramerate = targetFramerate;
    m_FrameDuration = 1.0 / targetFramerate;
    m_TotalFramesWritten = 0;
    m_TotalAudioWritten = 0.0;
    m_StartTime = std::chrono::steady_clock::now();
    m_FirstFrameReceived = false;
    m_LastDecision = SyncDecision::None;
    m_AudioWritesSinceLastVideo = 0;
    m_FirstVideoFrameWritten = false;

    g_pSystem->Debug->Log("[SyncSystem] Initialized: %.2f fps, frame duration=%.2fms",
        targetFramerate, m_FrameDuration * 1000.0);
}

void SyncSystem::OnAudioWritten(double durationSec) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_TotalAudioWritten += durationSec;
}

void SyncSystem::OnVideoWritten() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_TotalFramesWritten++;
    if (!m_FirstVideoFrameWritten) {
        m_FirstVideoFrameWritten = true;
        g_pSystem->Debug->Log("[SyncSystem] First video frame written at t=%.3fs",
            std::chrono::duration<double>(std::chrono::steady_clock::now() - m_StartTime).count());
    }
}

SyncDecision SyncSystem::DecideNext(const FrameData* videoFront, const AudioChunk* audioFront) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (!m_FirstVideoFrameWritten && videoFront != nullptr) {
        g_pSystem->Debug->Log("[SyncSystem] First video frame: forcing write");
        m_FirstFrameReceived = true;
        return SyncDecision::WriteVideo;
    }

    auto now = std::chrono::steady_clock::now();
    double elapsedRealTime = std::chrono::duration<double>(now - m_StartTime).count();
    double nextFrameTargetTime = m_TotalFramesWritten * m_FrameDuration;

    bool videoIsLate = (nextFrameTargetTime + m_FrameDuration * 2) < elapsedRealTime;

    if (audioFront == nullptr) {
        return DecideVideoOnly(videoFront, nextFrameTargetTime, elapsedRealTime);
    }

    if (videoFront == nullptr) {
        if (m_AudioWritesSinceLastVideo < 5) {
            m_AudioWritesSinceLastVideo++;
            return SyncDecision::WriteAudio;
        }
        return SyncDecision::None;
    }

    if (videoIsLate || m_AudioWritesSinceLastVideo >= 5) {
        m_AudioWritesSinceLastVideo = 0;
        return DecideVideoInternal(videoFront, nextFrameTargetTime, elapsedRealTime);
    }

    if (m_LastDecision == SyncDecision::WriteVideo) {
        m_AudioWritesSinceLastVideo++;
        m_LastDecision = SyncDecision::WriteAudio;
        return SyncDecision::WriteAudio;
    }
    else {
        m_LastDecision = DecideVideoInternal(videoFront, nextFrameTargetTime, elapsedRealTime);
        if (m_LastDecision == SyncDecision::WriteVideo) {
            m_AudioWritesSinceLastVideo = 0;
        }
        return m_LastDecision;
    }
}

SyncDecision SyncSystem::DecideVideoOnly(const FrameData* videoFront, double nextFrameTargetTime, double elapsedRealTime) {
    if (nextFrameTargetTime > elapsedRealTime) {
        return SyncDecision::None;
    }

    if (videoFront == nullptr) {
        return m_FirstVideoFrameWritten ? SyncDecision::RepeatVideo : SyncDecision::None;
    }

    m_FirstFrameReceived = true;

    if (!m_FirstVideoFrameWritten) {
        return SyncDecision::WriteVideo;
    }

    double timeDiff = (videoFront->RealTime - nextFrameTargetTime) * 1000.0;

    if (videoFront->RealTime < nextFrameTargetTime - (m_FrameDuration * 2)) {
        if (rand() % 100 < 5) {
            g_pSystem->Debug->Log("[SyncSystem] Dropping old frame: diff=%.2fms", timeDiff);
        }
        return SyncDecision::DropVideo;
    }

    if (videoFront->RealTime > nextFrameTargetTime + m_FrameDuration) {
        if (rand() % 100 < 5) {
            g_pSystem->Debug->Log("[SyncSystem] Repeating frame: diff=%.2fms", timeDiff);
        }
        return SyncDecision::RepeatVideo;
    }

    return SyncDecision::WriteVideo;
}

SyncDecision SyncSystem::DecideVideoInternal(const FrameData* videoFront, double nextFrameTargetTime, double elapsedRealTime) {
    if (nextFrameTargetTime > elapsedRealTime) {
        return SyncDecision::None;
    }

    if (videoFront == nullptr) {
        return m_FirstVideoFrameWritten ? SyncDecision::RepeatVideo : SyncDecision::None;
    }

    m_FirstFrameReceived = true;

    if (!m_FirstVideoFrameWritten) {
        return SyncDecision::WriteVideo;
    }

    double timeDiff = (videoFront->RealTime - nextFrameTargetTime) * 1000.0;

    if (videoFront->RealTime < nextFrameTargetTime - (m_FrameDuration * 2)) {
        return SyncDecision::DropVideo;
    }

    if (videoFront->RealTime > nextFrameTargetTime + m_FrameDuration) {
        return SyncDecision::RepeatVideo;
    }

    return SyncDecision::WriteVideo;
}