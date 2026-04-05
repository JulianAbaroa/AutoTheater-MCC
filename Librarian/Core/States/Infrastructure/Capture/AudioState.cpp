#include "pch.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"

bool AudioState::IsRecording() const { return m_IsRecording.load(); }
void AudioState::SetRecording(bool value) { m_IsRecording.store(value); }

bool AudioState::IsMuted() const { return m_IsMuted.load(); }
void AudioState::SetMuted(bool value) { m_IsMuted.store(value); }

AudioFormat AudioState::GetAudioInstance(void* instance) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_AudioInstances.find(instance);

    if (it != m_AudioInstances.end())
    {
        return it->second;
    }

    return AudioFormat{};
}

std::map<void*, AudioFormat> AudioState::GetAllAudioInstances() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_AudioInstances;
}

void AudioState::ClearAudioInstances()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_AudioInstances.clear();
    m_InstanceBuffers.clear();
    m_WasCleared.store(true);
}

BYTE* AudioState::GetBufferForInstance(void* instance)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_InstanceBuffers.find(instance);

    if (it != m_InstanceBuffers.end())
    {
        BYTE* buffer = it->second;
        m_InstanceBuffers.erase(it);
        return buffer;
    }
    return nullptr;
}

void AudioState::SetBufferForInstance(void* instance, BYTE* buffer)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_InstanceBuffers[instance] = buffer;
}


void AudioState::RegisterAudioInstance(void* instance, 
    WORD channels, DWORD samplesPerSec, WORD bytesPerFrame)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_AudioInstances[instance] = { channels, samplesPerSec, bytesPerFrame };
}

void AudioState::UnregisterAudioInstance(void* instance)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_AudioInstances.erase(instance);
    m_InstanceBuffers.erase(instance);
}

void AudioState::Cleanup()
{
    m_IsRecording.store(false);

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_AudioInstances.clear();
    m_InstanceBuffers.clear();
}