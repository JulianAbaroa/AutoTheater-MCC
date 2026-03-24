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

#include <audiosessiontypes.h>
#include <algorithm>

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

    {
        std::lock_guard<std::mutex> lock(m_InstancesMutex);
        m_ActiveInstances.clear();
    }

    this->Reset();
    m_LastMixTime = std::chrono::steady_clock::now();
    g_pState->Infrastructure->Audio->SetRecording(true);

    g_pSystem->Debug->Log("[AudioSystem] INFO: Audio recording started.");
}

void AudioSystem::StopRecording()
{
    this->ClearQueue();

    {
        std::lock_guard<std::mutex> lock(m_InstancesMutex);
        m_ActiveInstances.clear();
    }

    this->Reset();
    m_LastMixTime = {};
    g_pState->Infrastructure->Audio->SetRecording(false);

    g_pSystem->Debug->Log("[AudioSystem] INFO: Audio recording stopped.");
}


void AudioSystem::Update()
{
    auto now = std::chrono::steady_clock::now();
    if (now - m_LastMixTime >= m_MixInterval)
    {
        m_PendingMixInterval = std::chrono::duration<double>(now - m_LastMixTime).count();
        m_LastMixTime = now; 
        this->Mix();
    }

    this->CleanupInactiveInstances();
}

void AudioSystem::WriteAudio(void* instance, BYTE* pData, size_t size, bool isSilent)
{
    if (!g_pState->Domain->Theater->IsTheaterMode() || pData == nullptr || instance == nullptr) return;
    if (!g_pState->Infrastructure->Audio->IsRecording()) return;

    std::lock_guard<std::mutex> lock(m_InstancesMutex);

    auto it = std::find_if(m_ActiveInstances.begin(), m_ActiveInstances.end(),
        [instance](const ActiveInstance& ai) { return ai.Instance == instance; });

    if (it == m_ActiveInstances.end()) 
    {
        AudioFormat fmt = g_pState->Infrastructure->Audio->GetAudioInstance(instance);
        if (fmt.Channels == 8 || fmt.Channels == 2 || fmt.Channels == 6)
        {
            ActiveInstance newInst;
            newInst.Instance = instance;
            newInst.Format = fmt;
            newInst.LastDataTime = std::chrono::steady_clock::now();
            newInst.FirstDataTime = std::chrono::steady_clock::now();

            if (!m_ActiveInstances.empty()) {
                const auto& master = m_ActiveInstances[0];
                double masterElapsed = std::chrono::duration<double>(
                    newInst.FirstDataTime - master.FirstDataTime).count();

                if (masterElapsed > 0.0 && masterElapsed < 10.0) {
                    size_t silenceSamples = static_cast<size_t>(
                        masterElapsed
                        * static_cast<double>(fmt.SamplesPerSec)
                        * static_cast<double>(fmt.Channels));

                    newInst.PendingSamples.assign(silenceSamples, 0.0f);

                    g_pSystem->Debug->Log("[AudioSystem] INFO: New instance aligned to master"
                        " with %.3fs silence padding.",
                        instance, masterElapsed);
                }
            }

            m_ActiveInstances.push_back(newInst);
            it = m_ActiveInstances.end() - 1;

            g_pSystem->Debug->Log("[AudioSystem] INFO: New active instance.");
        }
        else 
        {
            return;
        }
    }

    it->LastDataTime = std::chrono::steady_clock::now();

    size_t numSamples = size / sizeof(float);
    size_t oldSize = it->PendingSamples.size();
    it->PendingSamples.resize(oldSize + numSamples);

    if (isSilent) 
    {
        std::fill(it->PendingSamples.begin() + oldSize, it->PendingSamples.end(), 0.0f);
    }
    else 
    {
        if (!SafeCopy(it->PendingSamples.data() + oldSize, pData, size)) 
        {
            g_pSystem->Debug->Log("[AudioSystem] ERROR: Memory access violation during SafeCopy.");
            m_ActiveInstances.erase(it);
            return;
        }
    }

    float* samples = it->PendingSamples.data() + oldSize;

    for (size_t i = 0; i < numSamples; ++i) 
    {
        float s = samples[i];

        if (!std::isfinite(s)) samples[i] = 0.0f;
        else if (std::abs(s) < 1e-10f) samples[i] = 0.0f;
        else if (s > 1.0f) samples[i] = 1.0f;
        else if (s < -1.0f) samples[i] = -1.0f;
    }
}


std::deque<AudioChunk> AudioSystem::ExtractQueue()
{
    std::lock_guard<std::mutex> lock(m_QueueMutex);
    std::deque<AudioChunk> ret;
    ret.swap(m_AudioQueue);
    return ret;
}

void AudioSystem::ClearQueue()
{
    std::lock_guard<std::mutex> lock(m_QueueMutex);
    m_AudioQueue.clear();
}


void AudioSystem::Reset()
{
    m_TotalSamplesMixed = 0;
    m_HasLastKnownFormat = false;
    m_SilenceFallbackAccumSec = 0.0;
    m_LastSelectedInstance = nullptr;
}

void AudioSystem::Cleanup()
{
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_AudioQueue.clear();
    }

    {
        std::lock_guard<std::mutex> lock(m_InstancesMutex);
        m_ActiveInstances.clear();
    }

    m_MixBuffer = {};
    m_LastMixTime = {};
    m_LastKnownFormat = {};
    m_TotalSamplesMixed = 0;
    m_PendingMixInterval = 0.0;
    m_HasLastKnownFormat = false;
    m_LastSelectedInstance = nullptr;
    m_SilenceFallbackAccumSec = 0.0;

    g_pState->Infrastructure->Audio->Cleanup();
    g_pSystem->Debug->Log("[AudioSystem] INFO: Audio cleanup completed.");
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


void AudioSystem::Mix()
{
    std::lock_guard<std::mutex> lock(m_InstancesMutex);

    double realIntervalSec = m_PendingMixInterval;
    if (realIntervalSec <= 0.0) return;

    bool isMuted = g_pState->Infrastructure->Audio->IsMuted();

    ActiveInstance* activeInst = nullptr;
    float selectedRMS = 0.0f;

    if (!isMuted && !m_ActiveInstances.empty())
    {
        const auto& fmt = m_ActiveInstances[0].Format;
        size_t targetSamples = static_cast<size_t>(
            realIntervalSec
            * static_cast<double>(fmt.SamplesPerSec)
            * static_cast<double>(fmt.Channels));
        targetSamples = (targetSamples / fmt.Channels) * fmt.Channels;

        for (auto& inst : m_ActiveInstances)
        {
            if (inst.PendingSamples.size() < targetSamples) continue;

            float rms = 0.0f;
            size_t check = (std::min)(targetSamples, (size_t)480);
            for (size_t i = 0; i < check; ++i)
                rms += inst.PendingSamples[i] * inst.PendingSamples[i];
            rms = std::sqrt(rms / check);

            if (rms > selectedRMS)
            {
                selectedRMS = rms;
                activeInst = &inst;
            }
        }
    }

    if (!m_ActiveInstances.empty())
    {
        m_LastKnownFormat = m_ActiveInstances[0].Format;
        m_HasLastKnownFormat = true;
    }

    if (!m_HasLastKnownFormat) return;

    const AudioFormat& fmt = m_LastKnownFormat;

    size_t targetSamples = static_cast<size_t>(
        realIntervalSec
        * static_cast<double>(fmt.SamplesPerSec)
        * static_cast<double>(fmt.Channels));
    targetSamples = (targetSamples / fmt.Channels) * fmt.Channels;

    if (targetSamples == 0) return;

    if (isMuted)
    {
        for (auto& inst : m_ActiveInstances)
        {
            size_t toDrain = (std::min)(inst.PendingSamples.size(), targetSamples);
            if (toDrain > 0)
                inst.PendingSamples.erase(inst.PendingSamples.begin(),
                    inst.PendingSamples.begin() + toDrain);
        }
        this->EmitSilenceChunk(fmt, targetSamples);
        m_SilenceFallbackAccumSec = 0.0;
        return;
    }

    if (activeInst != nullptr)
    {
        if (activeInst->Instance != m_LastSelectedInstance)
        {
            m_LastSelectedInstance = activeInst->Instance;
        }

        m_MixBuffer.assign(targetSamples, 0.0f);
        const float* src = activeInst->PendingSamples.data();
        for (size_t i = 0; i < targetSamples; ++i)
            m_MixBuffer[i] = src[i];

        float maxSample = 0.0f;
        for (float s : m_MixBuffer)
            if (std::abs(s) > maxSample) maxSample = std::abs(s);
        if (maxSample > 1.0f)
        {
            float scale = 1.0f / maxSample;
            for (float& s : m_MixBuffer) s *= scale;
        }

        for (auto& inst : m_ActiveInstances)
        {
            size_t toDrain = (std::min)(inst.PendingSamples.size(), targetSamples);
            if (toDrain > 0)
                inst.PendingSamples.erase(inst.PendingSamples.begin(),
                    inst.PendingSamples.begin() + toDrain);
        }

        if (m_SilenceFallbackAccumSec > 0.0)
        {
            g_pSystem->Debug->Log("[AudioSystem] INFO: Audio signal recovered after %.2fs of silence fallback.",
                m_SilenceFallbackAccumSec);
        }
        m_SilenceFallbackAccumSec = 0.0;

        uint64_t samplesBeforeMix = m_TotalSamplesMixed;
        m_TotalSamplesMixed += targetSamples;

        AudioChunk chunk;
        chunk.DurationSec = static_cast<double>(m_TotalSamplesMixed) / (fmt.SamplesPerSec * fmt.Channels)
            - static_cast<double>(samplesBeforeMix) / (fmt.SamplesPerSec * fmt.Channels);

        auto now = std::chrono::steady_clock::now();
        chunk.RealTime = std::chrono::duration<double>(
            now - g_pState->Infrastructure->FFmpeg->GetStartRecordingTime()).count();
        chunk.IsSilent = false;
        chunk.Data.resize(targetSamples * sizeof(float));
        memcpy(chunk.Data.data(), m_MixBuffer.data(), targetSamples * sizeof(float));

        {
            std::lock_guard<std::mutex> qlock(m_QueueMutex);
            m_AudioQueue.push_back(std::move(chunk));
        }
        return;
    }

    for (auto& inst : m_ActiveInstances)
    {
        size_t toDrain = (std::min)(inst.PendingSamples.size(), targetSamples);
        if (toDrain > 0)
            inst.PendingSamples.erase(inst.PendingSamples.begin(),
                inst.PendingSamples.begin() + toDrain);
    }

    m_SilenceFallbackAccumSec += realIntervalSec;

    if (m_SilenceFallbackAccumSec > k_MaxSilenceFallbackSec)
    {
        return;
    }

    if (m_SilenceFallbackAccumSec < 0.1)
    {
        g_pSystem->Debug->Log("[AudioSystem] WARNING: No active instance with signal,"
            " emitting silence fallback (limit=%.0fs).", k_MaxSilenceFallbackSec);
    }

    this->EmitSilenceChunk(fmt, targetSamples);
}


void AudioSystem::EmitSilenceChunk(const AudioFormat& fmt, size_t targetSamples)
{
    uint64_t samplesBeforeMix = m_TotalSamplesMixed;
    m_TotalSamplesMixed += targetSamples;

    AudioChunk chunk;
    chunk.DurationSec = static_cast<double>(m_TotalSamplesMixed) / (fmt.SamplesPerSec * fmt.Channels)
        - static_cast<double>(samplesBeforeMix) / (fmt.SamplesPerSec * fmt.Channels);

    auto now = std::chrono::steady_clock::now();
    chunk.RealTime = std::chrono::duration<double>(
        now - g_pState->Infrastructure->FFmpeg->GetStartRecordingTime()).count();
    chunk.IsSilent = true;
    chunk.Data.assign(targetSamples * sizeof(float), 0);

    {
        std::lock_guard<std::mutex> qlock(m_QueueMutex);
        m_AudioQueue.push_back(std::move(chunk));
    }
}

void AudioSystem::CleanupInactiveInstances()
{
    std::lock_guard<std::mutex> lock(m_InstancesMutex);
    auto now = std::chrono::steady_clock::now();

    constexpr auto kInactiveTimeout = std::chrono::milliseconds(1000);

    std::vector<void*> instancesToRemove;
    for (const auto& inst : m_ActiveInstances)
    {
        if ((now - inst.LastDataTime) > kInactiveTimeout)
            instancesToRemove.push_back(inst.Instance);
    }

    for (void* instance : instancesToRemove)
    {
        g_pSystem->Debug->Log("[AudioSystem] INFO: Removing stale instance.");
        g_pState->Infrastructure->Audio->UnregisterAudioInstance(instance);
    }

    m_ActiveInstances.erase(
        std::remove_if(m_ActiveInstances.begin(), m_ActiveInstances.end(),
            [now](const ActiveInstance& inst) {
                return (now - inst.LastDataTime) > kInactiveTimeout;
            }),
        m_ActiveInstances.end());
}