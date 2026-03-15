#include "pch.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"

bool AudioState::IsRecording() const { return m_IsRecording.load(); }
void AudioState::SetRecording(bool value) { m_IsRecording.store(value); }

void AudioState::SetBufferForInstance(void* instance, BYTE* buffer)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_InstanceBuffers[instance] = buffer;
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

void* AudioState::GetMasterInstance() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    void* master = m_MasterInstance.load();

    if (master != nullptr && m_AudioInstances.find(master) == m_AudioInstances.end())
    {
        return nullptr;
    }

    return master;
}

void AudioState::SetMasterInstance(void* newInstance)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_MasterInstance.store(newInstance);
}


void AudioState::RegisterAudioInstance(void* instance, WORD channels, DWORD samplesPerSec, WORD bytesPerFrame)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_AudioInstances[instance] = { channels, samplesPerSec, bytesPerFrame };
}

AudioFormat AudioState::GetAudioInstance(void* instance)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_AudioInstances.count(instance))
    {
        return m_AudioInstances[instance];
    }

    return AudioFormat{};
}

std::map<void*, AudioFormat> AudioState::GetAudioInstances()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_AudioInstances.empty()) return std::map<void*, AudioFormat>{};
    return m_AudioInstances;
}


void AudioState::Cleanup()
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_AudioInstances.clear();
        m_InstanceBuffers.clear();
        m_MasterInstance.store(nullptr);
    }

    m_IsRecording.store(false);
}