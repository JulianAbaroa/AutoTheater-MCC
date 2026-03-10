#pragma once

#include <vector>

// Stores 8-channel audio instances for master selection. 
// Instances with 'NaN' or 'inf' values are marked as invalid.
struct CandidateInfo
{
    void* Instance;
    bool IsInvalid;
};

// Buffers audio data with its corresponding engine timestamp 
// and silence detection state.
struct AudioChunk
{
    std::vector<BYTE> Data{};
    float EngineTime = 0.0f;
    bool IsSilent = false;
};

// Defines the specific audio configuration for an instance.
struct AudioFormat
{
    WORD Channels;
    DWORD SamplesPerSec;
    WORD BytesPerFrame;
};