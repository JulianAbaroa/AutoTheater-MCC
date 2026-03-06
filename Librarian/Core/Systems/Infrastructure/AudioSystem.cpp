#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
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
        __uuidof(MMDeviceEnumerator), 
        NULL, CLSCTX_ALL, 
        __uuidof(IMMDeviceEnumerator), 
        (void**)&pEnumerator
    );

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
        __uuidof(MMDeviceEnumerator), 
        NULL, CLSCTX_ALL, 
        __uuidof(IMMDeviceEnumerator), 
        (void**)&pEnumerator
    );

    if (SUCCEEDED(hr) && pEnumerator)
    {
        pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
        if (pDevice)
        {
            hr = pDevice->Activate(
                __uuidof(IAudioClient), 
                CLSCTX_ALL, NULL, 
                (void**)&pAudioClient
            );

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
    
    m_IsFirstCheck.store(true);
    g_pState->Audio.SetRecording(true);
    g_pUtil->Log.Append("[AudioSystem] INFO: Samples recording started.");
}

void AudioSystem::StopRecording() 
{
    this->ClearQueue();

    g_pState->Audio.SetRecording(false);
    g_pUtil->Log.Append("[AudioSystem] INFO: Samples recording stopped.");
}

void AudioSystem::Cleanup()
{
    this->ClearQueue();
    g_pState->Audio.Cleanup();
    g_pUtil->Log.Append("[AudioSystem] INFO: Cleanup and memory freed.");
}


void AudioSystem::ScrutinizeAudioInstance(void* instance, BYTE* pData, size_t size, bool isSilent)
{
    std::lock_guard<std::mutex> lock(m_ScrutinyMutex);

    if (g_pState->Audio.GetMasterInstance() != nullptr) return;

    CandidateInfo* current = nullptr;
    for (auto& c : m_Candidates)
    {
        if (c.instance == instance)
        {
            current = &c;
            break;
        }
    }

    if (current && current->isInvalid) return;

    auto const& activeInstances = g_pState->Audio.GetActiveInstances();
    auto itActive = activeInstances.find(instance);

    if (itActive == activeInstances.end() || itActive->second.Channels != 8)
    {
        if (itActive != activeInstances.end())
        {
            if (!current)
                m_Candidates.push_back({ instance, true });
            else
                current->isInvalid = true;
        }
        return;
    }

    const float* samples = reinterpret_cast<const float*>(pData);
    const size_t numSamples = size / sizeof(float);

    for (size_t i = 0; i < numSamples; ++i)
    {
        uint32_t u;
        memcpy(&u, &samples[i], 4);

        if ((u & 0x7F800000) == 0x7F800000)
        {
            if (!current)
                m_Candidates.push_back({ instance, true });
            else
                current->isInvalid = true;
            return;
        }
    }

    if (!current)
    {
        m_Candidates.push_back({ instance, false });
        current = &m_Candidates.back();
    }

    if (m_IsFirstCheck.load())
    {
        m_ScrutinyStart = std::chrono::steady_clock::now();
        m_IsFirstCheck.store(false);
    }

    float elapsed = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - m_ScrutinyStart).count();

    if (elapsed < 3.0f) return;

    void* potentialMaster = nullptr;
    int survivorCount = 0;

    for (auto& c : m_Candidates)
    {
        if (!c.isInvalid)
        {
            potentialMaster = c.instance;
            survivorCount++;
        }
    }

    if (survivorCount == 1)
    {
        g_pState->Audio.SetMasterInstance(potentialMaster);
        m_Candidates.clear();
        m_IsFirstCheck.store(true);
        g_pUtil->Log.Append("[AudioSystem] INFO: Master instance selected.");
    }
    else
    {
        g_pUtil->Log.Append(survivorCount == 0
            ? "[AudioSystem] WARNING: No valid candidates after scrutiny, restarting."
            : "[AudioSystem] WARNING: Multiple candidates survived scrutiny, restarting.");
        m_Candidates.clear();
        m_IsFirstCheck.store(true);
    }
}

void AudioSystem::WriteAudio(void* instance, BYTE* pData, size_t size, bool isSilent)
{
    if (!g_pState->Theater.IsTheaterMode() || pData == nullptr || instance == nullptr) return;

    void* masterInstance = g_pState->Audio.GetMasterInstance();
    if (masterInstance == nullptr)
    {
        this->ScrutinizeAudioInstance(instance, pData, size, isSilent);
        return;
    }

    if (instance == masterInstance)
    {
        AudioChunk chunk;
        float* pTime = g_pState->Theater.GetTimePtr();
        chunk.engineTime = (pTime) ? *pTime : 0.0f;
        chunk.isSilent = isSilent;

        chunk.data.resize(size);

        if (!isSilent && pData != nullptr)
        {
            memcpy(chunk.data.data(), pData, size);

            float* samples = reinterpret_cast<float*>(chunk.data.data());
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
            memset(chunk.data.data(), 0, size);
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

size_t AudioSystem::GetQueueSize()
{
    std::lock_guard<std::mutex> lock(m_QueueMutex);
    if (m_AudioQueue.empty()) return 0;
    return m_AudioQueue.size();
}

void AudioSystem::ClearQueue()
{
    std::lock_guard<std::mutex> lock(m_QueueMutex);
    if (m_AudioQueue.empty()) return;
    m_AudioQueue.clear();
}