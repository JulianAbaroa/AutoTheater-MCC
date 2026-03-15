#pragma once

#include "Core/Common/Types/AudioTypes.h"
#include "Core/Common/Types/VideoTypes.h"
#include "Core/Common/Types/SyncTypes.h"
#include <cstdint>
#include <chrono>
#include <deque>
#include <mutex>

class SyncSystem {
public:
    void InitializeBaseline(double targetFramerate);
    void OnAudioWritten(double durationSec);
    void OnVideoWritten();
    SyncDecision DecideNext(const FrameData* videoFront, const AudioChunk* audioFront);

private:
    SyncDecision DecideVideoOnly(const FrameData* videoFront, double nextFrameTargetTime, double elapsedRealTime);
    SyncDecision DecideVideoInternal(const FrameData* videoFront, double nextFrameTargetTime, double elapsedRealTime);

    std::mutex m_Mutex;
    double m_TargetFramerate = 60.0;
    double m_FrameDuration = 1.0 / 60.0;
    uint64_t m_TotalFramesWritten = 0;
    double m_TotalAudioWritten = 0.0;
    std::chrono::steady_clock::time_point m_StartTime;
    bool m_FirstFrameReceived = false;
    bool m_FirstVideoFrameWritten = false;

    SyncDecision m_LastDecision = SyncDecision::None;
    int m_AudioWritesSinceLastVideo = 0;
};