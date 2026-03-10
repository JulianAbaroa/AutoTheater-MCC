#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"

BYTE* AudioState::GetLastBuffer() const { return m_LastBuffer.load(); }
bool AudioState::IsRecording() const { return m_IsRecording.load(); }

void AudioState::SetLastBuffer(BYTE* pBuffer) { m_LastBuffer.store(pBuffer); }
void AudioState::SetRecording(bool value) { m_IsRecording.store(value); }

void* AudioState::GetMasterInstance() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_MasterInstance.load();
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
        m_MasterInstance.store(nullptr);
    }

    m_IsRecording.store(false);
}