#include "pch.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/AudioSystem.h"
#include "Core/Systems/Infrastructure/Capture/SyncSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <mmdeviceapi.h>
#include <chrono>

void* AudioSystem::GetRenderClientVTableAddress(int index)
{
    HRESULT hr = CoInitialize(NULL);
    if (!SUCCEEDED(hr)) return nullptr;

    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;
    IAudioRenderClient* pRenderClient = NULL;
    WAVEFORMATEX* pwfx = NULL;
    void* functionAddress = nullptr;

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, 
        __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);

    if (SUCCEEDED(hr))
    {
        if (pEnumerator) 
        {
            pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
            if (pDevice) 
            {
                pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);

                if (pAudioClient) 
                {
                    pAudioClient->GetMixFormat(&pwfx);
                    pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, pwfx, NULL);
                    pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient);

                    if (pRenderClient) 
                    {
                        void** vtable = *(void***)pRenderClient;
                        functionAddress = vtable[index];
                        pRenderClient->Release();
                    }

                    CoTaskMemFree(pwfx);
                    pAudioClient->Release();
                }

                pDevice->Release();

            }

            pEnumerator->Release();
        }
    }

    CoUninitialize();
    return functionAddress;
}

void* AudioSystem::GetAudioClientVTableAddress(int index)
{
    HRESULT hr = CoInitialize(NULL);
    void* functionAddress = nullptr;

    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, 
        __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);

    if (SUCCEEDED(hr) && pEnumerator)
    {
        pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
        if (pDevice)
        {
            hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);

            if (SUCCEEDED(hr) && pAudioClient)
            {
                void** vtable = *(void***)pAudioClient;
                functionAddress = vtable[index];
                pAudioClient->Release();
            }

            pDevice->Release();
        }

        pEnumerator->Release();
    }

    CoUninitialize();
    return functionAddress;
}


void AudioSystem::StartRecording() 
{
    this->ClearQueue();
    m_IsScrutinyStarted.store(false);
    g_pState->Infrastructure->Audio->SetRecording(true);
    g_pSystem->Debug->Log("[AudioSystem] INFO: Audio recording started.");
}

void AudioSystem::StopRecording() 
{
    this->ClearQueue();
    g_pState->Infrastructure->Audio->SetRecording(false);
    g_pSystem->Debug->Log("[AudioSystem] INFO: Audio recording stopped.");
}


void AudioSystem::WriteAudio(void* instance, BYTE* pData, size_t size, bool isSilent)
{
    if (!g_pState->Domain->Theater->IsTheaterMode() || pData == nullptr || instance == nullptr) return;

    void* masterInstance = g_pState->Infrastructure->Audio->GetMasterInstance();
    if (masterInstance == nullptr)
    {
        this->ScrutinizeAudioInstance(instance, pData, size, isSilent);
        return;
    }

    if (instance == masterInstance)
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - g_pState->Infrastructure->FFmpeg->GetStartRecordingTime();
        double currentRealTime = elapsed.count();

        AudioChunk chunk;
        chunk.RealTime = currentRealTime;
        chunk.IsSilent = isSilent;
        chunk.Data.resize(size);

        static bool logged = false;
        if (!logged)
        {
            logged = true;
            auto fmt = g_pState->Infrastructure->Audio->GetAudioInstance(instance);
            g_pSystem->Debug->Log("[AudioSystem] FORMAT: SamplesPerSec=%u, Channels=%u, BitsPerSample=%u, BytesPerFrame=%u",
                fmt.SamplesPerSec, fmt.Channels, fmt.SamplesPerSec, fmt.BytesPerFrame);
            g_pSystem->Debug->Log("[AudioSystem] FORMAT: chunk size=%zu bytes, frames_per_chunk=%zu",
                size, size / fmt.BytesPerFrame);
        }

        if (isSilent)
        {
            std::fill(chunk.Data.begin(), chunk.Data.end(), 0);
        }
        else
        {
            if (!SafeCopy(chunk.Data.data(), pData, size))
            {
                g_pSystem->Debug->Log("[AudioSystem] ERROR: Memory access violation during SafeCopy.");
                g_pState->Infrastructure->Audio->SetMasterInstance(nullptr);
                return;
            }

            float* samples = reinterpret_cast<float*>(chunk.Data.data());
            size_t numSamples = size / sizeof(float);

            for (size_t i = 0; i < numSamples; ++i)
            {
                float s = samples[i];

                if (!std::isfinite(s))
                {
                    samples[i] = 0.0f;
                    continue;
                }

                if (std::abs(s) < 1e-10f)
                {
                    samples[i] = 0.0f;
                    continue;
                }

                if (s > 1.0f) samples[i] = 1.0f;
                else if (s < -1.0f) samples[i] = -1.0f;
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            m_AudioQueue.push_back(std::move(chunk));
        }
    }
}


std::deque<AudioChunk> AudioSystem::ExtractQueue()
{
    std::deque<AudioChunk> localQueue;
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        if (m_AudioQueue.empty()) return localQueue;
        localQueue.swap(m_AudioQueue);
    }

    return localQueue;
}

void AudioSystem::ClearQueue()
{
    std::lock_guard<std::mutex> lock(m_QueueMutex);
    if (m_AudioQueue.empty()) return;
    m_AudioQueue.clear();

    g_pSystem->Debug->Log("[AudioSystem] INFO: Audio queue cleared.");
}


void AudioSystem::Cleanup()
{
    this->ClearQueue();
    g_pState->Infrastructure->Audio->Cleanup();

    std::lock_guard<std::mutex> lock(m_ScrutinyMutex);
    m_Candidates.clear();
    m_IsScrutinyStarted.store(false);
    m_ScrutinyStart = std::chrono::steady_clock::time_point();

    g_pSystem->Debug->Log("[AudioSystem] INFO: Audio cleanup completed.");
}


void AudioSystem::ScrutinizeAudioInstance(void* instance, BYTE* pData, size_t size, bool isSilent)
{
    if (g_pState->Infrastructure->Audio->GetMasterInstance() != nullptr) return;

    auto* pTheaterTime = g_pState->Domain->Theater->GetTimePtr();
    if (pTheaterTime == nullptr || *pTheaterTime < 0.0f) return;

    std::lock_guard<std::mutex> lock(m_ScrutinyMutex);

    CandidateInfo* currentCandidate = nullptr;
    for (auto& candidate : m_Candidates)
    {
        if (candidate.Instance == instance)
        {
            currentCandidate = &candidate;
            break;
        }
    }

    if (currentCandidate && currentCandidate->IsInvalid) return;

    auto const& audioInstances = g_pState->Infrastructure->Audio->GetAudioInstances();
    auto itActive = audioInstances.find(instance);
    if (itActive == audioInstances.end() || itActive->second.Channels != 8)
    {
        this->MarkAsInvalid(instance, currentCandidate);
        return;
    }

    const float* samples = reinterpret_cast<const float*>(pData);
    const size_t numSamples = size / sizeof(float);
    bool chunkHasActivity = false;
    const float activityThreshold = 0.00001f; // TODO: maybe incrase this.

    for (size_t i = 0; i < numSamples; ++i)
    {
        float s = samples[i];
        const uint32_t sampleBits = *(reinterpret_cast<const uint32_t*>(&s));

        if ((sampleBits & m_IEE754NanInfMask) == m_IEE754NanInfMask)
        {
            this->MarkAsInvalid(instance, currentCandidate);
            return;
        }

        if (!chunkHasActivity && std::abs(s) > activityThreshold)
        {
            chunkHasActivity = true;
        }
    }

    if (!currentCandidate)
    {
        m_Candidates.push_back({ instance, false });
        currentCandidate = &m_Candidates.back();
        currentCandidate->HasHadActivity = false;
    }

    if (chunkHasActivity)
    {
        currentCandidate->HasHadActivity = true;
    }

    if (!m_IsScrutinyStarted.load())
    {
        m_ScrutinyStart = std::chrono::steady_clock::now();
        m_IsScrutinyStarted.store(true);
    }

    auto now = std::chrono::steady_clock::now();
    if ((now - m_ScrutinyStart) < m_ScrutinyThreshold) return;

    void* potentialMaster = nullptr;
    int survivorCount = 0;

    for (auto& candidate : m_Candidates)
    {
        if (!candidate.IsInvalid && candidate.HasHadActivity)
        {
            potentialMaster = candidate.Instance;
            survivorCount++;
        }
    }

    if (survivorCount == 1)
    {
        g_pState->Infrastructure->Audio->SetMasterInstance(potentialMaster);
        g_pSystem->Debug->Log("[AudioSystem] INFO: Master instance selected by Activity Scrutiny.");
        m_Candidates.clear();
        m_IsScrutinyStarted.store(false);
    }
    else
    {
        g_pSystem->Debug->Log("[AudioSystem] WARNING: Scrutiny Failed: %d survivors (Activity required). Restarting...", survivorCount);
        m_Candidates.clear();
        m_IsScrutinyStarted.store(false);
    }
}

void AudioSystem::MarkAsInvalid(void* instance, CandidateInfo* currentCandidate)
{
    if (!currentCandidate) m_Candidates.push_back({ instance, true });
    else currentCandidate->IsInvalid = true;
}

bool AudioSystem::SafeCopy(void* dest, const void* src, size_t size)
{
    __try
    {
        if (src == nullptr || dest == nullptr) return false;
        memcpy(dest, src, size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}