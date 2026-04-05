#pragma once

#include "Core/Common/Types/AudioTypes.h"
#include <chrono>
#include <mutex>
#include <deque>

using namespace std::chrono_literals;

class AudioSystem {
public:
    void StartRecording();
    void StopRecording();

    void WriteAudio(void* instance, BYTE* pData, size_t size, bool isSilent);
    void Update();

    void FlushPendingSamples();

    std::deque<AudioChunk> ExtractQueue();
    void ClearQueue();

    void Cleanup();

private:
    void Reset();

    void Mix();
    void CleanupInactiveInstances();
    void EmitSilenceChunk(const AudioFormat& fmt, size_t targetSamples);

    bool SafeCopy(void* dest, const void* src, size_t size);

    std::deque<AudioChunk> m_AudioQueue;
    std::mutex m_QueueMutex;

    std::vector<ActiveInstance> m_ActiveInstances;
    std::mutex m_ActiveInstancesMutex;

    // Mix.
    std::vector<float> m_MixBuffer{};
    std::chrono::steady_clock::time_point m_LastMixTime;
	std::mutex m_MixMutex;

    std::atomic<uint64_t> m_TotalSamplesMixed{ 0 };
    std::atomic<double> m_PendingMixInterval{ 0.0 };

    static constexpr auto m_MixInterval = 10ms;
    static constexpr auto m_InactiveTimeout = 1000ms;

    AudioFormat m_LastKnownFormat{};

    std::atomic<bool> m_HasLastKnownFormat{ false };
    std::atomic<void*> m_LastSelectedInstance{ nullptr };

    std::atomic<double> m_SilenceFallbackAccumSec{ 0.0 };
    static constexpr double k_MaxSilenceFallbackSec = 5.0;
};