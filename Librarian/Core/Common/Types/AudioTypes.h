#pragma once

#include <vector>
#include <chrono>

// Buffers audio data with its corresponding engine timestamp 
// and silence detection state.
struct AudioChunk
{
    std::vector<BYTE> Data{};
    double RealTime = 0.0;
    double DurationSec = 0.0;
    bool IsSilent = false;
};

// Defines the specific audio configuration for an instance.
struct AudioFormat
{
    WORD Channels;
    DWORD SamplesPerSec;
    WORD BytesPerFrame;
};

struct ActiveInstance 
{
    void* Instance = nullptr;
    AudioFormat Format{};
    std::chrono::steady_clock::time_point LastDataTime{};
    std::chrono::steady_clock::time_point FirstDataTime{};
    std::vector<float> PendingSamples;
};