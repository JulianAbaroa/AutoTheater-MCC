#pragma once

#include "Core/Common/Types/AudioTypes.h"
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <map>

class AudioState
{
public:
    bool IsRecording() const;
    void SetRecording(bool value);

    bool IsMuted() const;
    void SetMuted(bool value);

    AudioFormat GetAudioInstance(void* instance) const;
    std::map<void*, AudioFormat> GetAllAudioInstances() const;
    void ClearAudioInstances();

    BYTE* GetBufferForInstance(void* instance);
    void SetBufferForInstance(void* instance, BYTE* buffer);

    void RegisterAudioInstance(void* instance, WORD channels, DWORD samplesPerSec, WORD bytesPerFrame);
    void UnregisterAudioInstance(void* instance);
    
    void Cleanup();

private:
    std::atomic<bool> m_IsRecording{ false };
    std::atomic<bool> m_IsMuted{ false };
    std::atomic<bool> m_WasCleared{ false };

    std::map<void*, AudioFormat> m_AudioInstances{};
    std::unordered_map<void*, BYTE*> m_InstanceBuffers{};
    mutable std::mutex m_Mutex;
};