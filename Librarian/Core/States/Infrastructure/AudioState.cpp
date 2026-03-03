#include "pch.h"
#include "Core/States/Infrastructure/AudioState.h"

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

void AudioState::ResetMasterInstance() 
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_MasterInstance.store(nullptr);
}

BYTE* AudioState::GetLastBuffer() const { return m_LastBuffer.load(); }
void AudioState::SetLastBuffer(BYTE* pBuffer) { m_LastBuffer.store(pBuffer); }

bool AudioState::IsRecording() const { return m_IsRecording.load(); }
void AudioState::SetRecording(bool value) { m_IsRecording.store(value); }

void AudioState::RegisterActiveInstance(void* instance, WORD channels, DWORD samplesPerSec, WORD bytesPerFrame)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ActiveInstances[instance] = { channels, samplesPerSec, bytesPerFrame };
}

AudioFormat AudioState::GetActiveInstance(void* instance)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_ActiveInstances.count(instance))
    {
        return m_ActiveInstances[instance];
    }

    return AudioFormat{};
}

std::map<void*, AudioFormat> AudioState::GetActiveInstances()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_ActiveInstances.empty()) return std::map<void*, AudioFormat>{};
    return m_ActiveInstances;
}

void AudioState::ClearActiveInstances()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ActiveInstances.clear();
}


void AudioState::ResetForRecording()
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_MasterInstance.store(nullptr);
    }

    m_IsRecording.store(false);
}

void AudioState::Cleanup()
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_ActiveInstances.clear();
        m_MasterInstance.store(nullptr);
    }

    m_IsRecording.store(false);
}