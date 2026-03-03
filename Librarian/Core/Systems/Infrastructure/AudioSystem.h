#pragma once

#include "Core/Common/Types/AudioTypes.h"
#include <Audioclient.h>
#include <mutex>

class AudioSystem
{
public:
    void* GetRenderClientVTableAddress(int index);
    void* GetAudioClientVTableAddress(int index);

    void StartRecording();
    void StopRecording();
    void Cleanup();

    void WriteAudio(void* instance, BYTE* pData, size_t size, bool isSilent);
    
    std::deque<AudioChunk> ExtractQueue();
    size_t GetQueueSize();
    void ClearQueue();

private:
    void ScrutinizeAudioInstance(void* instance, BYTE* pData, size_t size, bool isSilent);

    std::deque<AudioChunk> m_AudioQueue;
    std::mutex m_QueueMutex;

    std::vector<CandidateInfo> m_Candidates{ 16 };
    std::atomic<bool> m_IsFirstCheck{ true };
    std::chrono::steady_clock::time_point m_ScrutinyStart;
    std::mutex m_ScrutinyMutex;
};