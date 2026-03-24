#pragma once

#include "Core/Common/Types/AudioTypes.h"
#include <Audioclient.h>
#include <mutex>
#include <deque>

class AudioSystem {
public:
    void* GetRenderClientVTableAddress(int index);
    void* GetAudioClientVTableAddress(int index);

    void StartRecording();
    void StopRecording();

    void Update();
    void WriteAudio(void* instance, BYTE* pData, size_t size, bool isSilent);

    std::deque<AudioChunk> ExtractQueue();
    void ClearQueue();

    void Cleanup();

private:
    void Mix();
    void Reset();
    void CleanupInactiveInstances();
    bool SafeCopy(void* dest, const void* src, size_t size);
    void EmitSilenceChunk(const AudioFormat& fmt, size_t targetSamples);

    std::deque<AudioChunk> m_AudioQueue;
    std::mutex m_QueueMutex;

    std::vector<ActiveInstance> m_ActiveInstances;
    std::mutex m_InstancesMutex;

    void* m_LastSelectedInstance = nullptr;

    // Mix.
    uint64_t m_TotalSamplesMixed = 0;
    std::vector<float> m_MixBuffer;
    std::chrono::steady_clock::time_point m_LastMixTime;
    static constexpr auto m_MixInterval = std::chrono::milliseconds(10);
    double m_PendingMixInterval = 0.0;

    AudioFormat m_LastKnownFormat{};
    bool m_HasLastKnownFormat = false;

    double m_SilenceFallbackAccumSec = 0.0;
    static constexpr double k_MaxSilenceFallbackSec = 5.0;
};