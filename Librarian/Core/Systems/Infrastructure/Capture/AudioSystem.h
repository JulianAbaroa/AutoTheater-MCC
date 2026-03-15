#pragma once

#include "Core/Common/Types/AudioTypes.h"
#include <Audioclient.h>
#include <mutex>
#include <deque>

class AudioSystem
{
public:
    void* GetRenderClientVTableAddress(int index);
    void* GetAudioClientVTableAddress(int index);

    void StartRecording();
    void StopRecording();

    void WriteAudio(void* instance, BYTE* pData, size_t size, bool isSilent);

    std::deque<AudioChunk> ExtractQueue();
    void ClearQueue();

    void Cleanup();

private:
    void ScrutinizeAudioInstance(void* instance, BYTE* pData, size_t size, bool isSilent);
    void MarkAsInvalid(void* instance, CandidateInfo* currentCandidate);
    bool SafeCopy(void* dest, const void* src, size_t size);

    std::deque<AudioChunk> m_AudioQueue;
    std::mutex m_QueueMutex;

    std::vector<CandidateInfo> m_Candidates{};
    std::atomic<bool> m_IsScrutinyStarted{ true };
    std::chrono::steady_clock::time_point m_ScrutinyStart;
    std::mutex m_ScrutinyMutex;

    static constexpr uint32_t m_IEE754NanInfMask = 0x7F800000;
    static constexpr std::chrono::duration<float> m_ScrutinyThreshold = std::chrono::seconds(3);
};