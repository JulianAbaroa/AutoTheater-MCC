#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/Capture/AudioSystem.h"
#include <mmdeviceapi.h>

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
    g_pUtil->Log.Append("[AudioSystem] INFO: Samples recording started.");
}

void AudioSystem::StopRecording() 
{
    this->ClearQueue();
    g_pState->Infrastructure->Audio->SetRecording(false);
    g_pUtil->Log.Append("[AudioSystem] INFO: Samples recording stopped.");
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
        AudioChunk chunk;
        float* pTime = g_pState->Domain->Theater->GetTimePtr();
        chunk.EngineTime = (pTime) ? *pTime : 0.0f;
        chunk.IsSilent = isSilent;

        chunk.Data.resize(size);

        if (!isSilent && pData != nullptr)
        {
            memcpy(chunk.Data.data(), pData, size);

            float* samples = reinterpret_cast<float*>(chunk.Data.data());
            size_t numSamples = size / sizeof(float);

            for (size_t i = 0; i < numSamples; ++i) 
            {
                if (!std::isfinite(samples[i])) samples[i] = 0.0f;
                else if (samples[i] > 1.0f) samples[i] = 1.0f;
                else if (samples[i] < -1.0f) samples[i] = -1.0f;
            }
        }
        else 
        {
            memset(chunk.Data.data(), 0, size);
        }

        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_AudioQueue.push_back(std::move(chunk));
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
}


void AudioSystem::Cleanup()
{
    this->ClearQueue();
    g_pState->Infrastructure->Audio->Cleanup();
    g_pUtil->Log.Append("[AudioSystem] INFO: Cleanup completed.");
}


void AudioSystem::ScrutinizeAudioInstance(void* instance, BYTE* pData, size_t size, bool isSilent)
{
    if (g_pState->Infrastructure->Audio->GetMasterInstance() != nullptr) return;

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

    // Filter: Exists & Channels.
    if (currentCandidate && currentCandidate->IsInvalid) return;

    auto const& audioInstances = g_pState->Infrastructure->Audio->GetAudioInstances();
    auto itActive = audioInstances.find(instance);

    if (itActive == audioInstances.end()) return;

    if (itActive->second.Channels != 8)
    {
        this->MarkAsInvalid(instance, currentCandidate);
        return;
    }

    // Filter: Nan or Inf.
    const float* samples = reinterpret_cast<const float*>(pData);
    const size_t numSamples = size / sizeof(float);

    for (size_t i = 0; i < numSamples; ++i)
    {
        const uint32_t sampleBits = *(reinterpret_cast<const uint32_t*>(&samples[i]));

        // Check if the sample's exponent bits are all set (0x7F800000).
        // According to IEEE 754, if the exponent is 0xFF (all 1s), the value is either NaN or Infinity.
        if ((sampleBits & m_IEE754NanInfMask) == m_IEE754NanInfMask)
        {
            this->MarkAsInvalid(instance, currentCandidate);
            return;
        }
    }

    // Mark as valid.
    if (!currentCandidate)
    {
        m_Candidates.push_back({ instance, false });
        currentCandidate = &m_Candidates.back();
    }

    // Scrutiny.
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
        if (!candidate.IsInvalid)
        {
            potentialMaster = candidate.Instance;
            survivorCount++;
        }
    }

    if (survivorCount == 1)
    {
        g_pState->Infrastructure->Audio->SetMasterInstance(potentialMaster);

        m_Candidates.clear();
        m_IsScrutinyStarted.store(false);
        g_pUtil->Log.Append("[AudioSystem] INFO: Master instance selected.");
    }
    else
    {
        g_pUtil->Log.Append(survivorCount == 0
            ? "[AudioSystem] WARNING: No valid candidates after scrutiny, restarting."
            : "[AudioSystem] WARNING: Multiple candidates survived scrutiny, restarting.");

        m_Candidates.clear();
        m_IsScrutinyStarted.store(false);
    }
}

void AudioSystem::MarkAsInvalid(void* instance, CandidateInfo* currentCandidate)
{
    if (!currentCandidate) m_Candidates.push_back({ instance, true });
    else currentCandidate->IsInvalid = true;
}