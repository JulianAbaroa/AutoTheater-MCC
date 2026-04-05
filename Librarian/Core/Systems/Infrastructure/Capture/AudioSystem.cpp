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

void AudioSystem::StartRecording()
{
    {
		std::lock_guard<std::mutex> lock(m_MixMutex);
        m_LastMixTime = std::chrono::steady_clock::now();
    }

    g_pState->Infrastructure->Audio->SetRecording(true);
    g_pSystem->Debug->Log("[AudioSystem] INFO: Audio recording started.");
}

void AudioSystem::StopRecording()
{
    this->Reset();
    g_pState->Infrastructure->Audio->SetRecording(false);
    g_pSystem->Debug->Log("[AudioSystem] INFO: Audio recording stopped.");
}


void AudioSystem::WriteAudio(void* instance, BYTE* pData, size_t size, bool isSilent)
{
	bool isTheaterMode = g_pState->Domain->Theater->IsTheaterMode();
	bool isRecording = g_pState->Infrastructure->Audio->IsRecording();

    if (!isTheaterMode || !isRecording || 
        pData == nullptr || instance == nullptr) return;

    std::lock_guard<std::mutex> lock(m_ActiveInstancesMutex);

    // Find the current audio instance in our active instances.
    auto it = std::find_if(m_ActiveInstances.begin(), m_ActiveInstances.end(),
        [instance](const ActiveInstance& ai) { return ai.Instance == instance; });

    // If we don't have this instance inside our active instances.
    if (it == m_ActiveInstances.end()) 
    {
        AudioFormat format = g_pState->Infrastructure->Audio->GetAudioInstance(instance);

        if (format.Channels == 8 || format.Channels == 2 || format.Channels == 6)
        {
            ActiveInstance newInstance;
            newInstance.Instance = instance;
            newInstance.Format = format;
            newInstance.LastDataTime = std::chrono::steady_clock::now();
            newInstance.FirstDataTime = std::chrono::steady_clock::now();

            // If this is not the first active instance encountered.
            if (!m_ActiveInstances.empty()) 
            {
                // We get the first active instance encountered.
                const auto& firstInstance = m_ActiveInstances[0];

				// We calculate the elapsed time between the first and the new instance.
                double firstElapsed = std::chrono::duration<double>(
                    newInstance.FirstDataTime - firstInstance.FirstDataTime).count();

                if (firstElapsed > 0.0 && firstElapsed < 10.0) 
                {
                    // We get the needed silence samples to align the new instance to the first one.
                    size_t silenceSamples = static_cast<size_t>(firstElapsed * 
                        static_cast<double>(format.SamplesPerSec)
                        * static_cast<double>(format.Channels));

                    newInstance.PendingSamples.assign(silenceSamples, 0.0f);

                    g_pSystem->Debug->Log("[AudioSystem] INFO: New instance aligned to master"
                        " with %.3fs silence padding.", instance, firstElapsed);
                }
            }

            m_ActiveInstances.push_back(newInstance);
            it = m_ActiveInstances.end() - 1;

            g_pSystem->Debug->Log("[AudioSystem] INFO: New active instance.");
        }
        else 
        {
            g_pSystem->Debug->Log("[AudioSystem] WARNING: Audio instance not supported.");
            return;
        }
    }

    it->LastDataTime = std::chrono::steady_clock::now();

    size_t numSamples = size / sizeof(float);
    size_t oldSize = it->PendingSamples.size();
    it->PendingSamples.resize(oldSize + numSamples);

    if (isSilent) 
    {
        // We fill with zeros.
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

		// We handle NaN/Inf, remove sub-perceptual noise and hard-limit to [-1.0, 1.0].
        if (!std::isfinite(s)) samples[i] = 0.0f;
        else if (std::abs(s) < 1e-10f) samples[i] = 0.0f;
        else if (s > 1.0f) samples[i] = 1.0f;
        else if (s < -1.0f) samples[i] = -1.0f;
    }
}

void AudioSystem::Update()
{
    auto now = std::chrono::steady_clock::now();

    {
		std::lock_guard<std::mutex> lock(m_MixMutex);
        if (now - m_LastMixTime >= m_MixInterval)
        {
            m_PendingMixInterval.store(std::chrono::duration<double>(now - m_LastMixTime).count());
            m_LastMixTime = now;
            this->Mix();
        }
    }

    this->CleanupInactiveInstances();
}


void AudioSystem::FlushPendingSamples()
{
    std::lock_guard<std::mutex> lock(m_ActiveInstancesMutex);
    for (auto& inst : m_ActiveInstances)
    {
        inst.PendingSamples.clear();
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


void AudioSystem::Cleanup()
{
    this->Reset();
    g_pState->Infrastructure->Audio->Cleanup();
    g_pSystem->Debug->Log("[AudioSystem] INFO: Audio cleanup completed.");
}


void AudioSystem::Reset()
{
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
		m_AudioQueue.clear();
    }

    {
        std::lock_guard<std::mutex> lock(m_ActiveInstancesMutex);
        m_ActiveInstances.clear();
    }

    // Mix.
    {
		std::lock_guard<std::mutex> lock(m_MixMutex);
        m_MixBuffer = {};
        m_LastMixTime = {};
    
        m_LastKnownFormat = {};
    }

    m_TotalSamplesMixed.store(0);
    m_PendingMixInterval.store(0.0);

    m_HasLastKnownFormat.store(false);
    m_LastSelectedInstance.store(nullptr);

    m_SilenceFallbackAccumSec.store(0.0);

	g_pSystem->Debug->Log("[AudioSystem] INFO: Reset completed.");
}


void AudioSystem::Mix()
{
    std::lock_guard<std::mutex> lock(m_ActiveInstancesMutex);

    // We get the elapsed time since the last mix to know how many samples are needed.
    double realIntervalSec = m_PendingMixInterval.load();
    double maxInterval = std::chrono::duration<double>(m_MixInterval).count() * 2.0;

    if (realIntervalSec > maxInterval) realIntervalSec = maxInterval;
    if (realIntervalSec <= 0.0) return;

    ActiveInstance* currentInstance = nullptr;
    float selectedRMS = 0.0f;

    bool isMuted = g_pState->Infrastructure->Audio->IsMuted();
    if (!isMuted && !m_ActiveInstances.empty())
    {
        const auto& format = m_ActiveInstances[0].Format;

        size_t targetSamples = static_cast<size_t>(realIntervalSec * 
            static_cast<double>(format.SamplesPerSec) * 
            static_cast<double>(format.Channels));

        targetSamples = (targetSamples / format.Channels) * format.Channels;

        for (auto& activeInstance : m_ActiveInstances)
        {
            if (activeInstance.PendingSamples.size() < targetSamples) continue;

            float rms = 0.0f;
            size_t check = (std::min)(targetSamples, (size_t)480);

            for (size_t i = 0; i < check; ++i)
            {
                rms += activeInstance.PendingSamples[i] * activeInstance.PendingSamples[i];
            }

            rms = std::sqrt(rms / check);

            if (rms > selectedRMS)
            {
                selectedRMS = rms;
                currentInstance = &activeInstance;
            }
        }
    }

    // Store the last known format so we can emit silence even if all instances disappear.
    if (!m_ActiveInstances.empty())
    {
        m_LastKnownFormat = m_ActiveInstances[0].Format;
        m_HasLastKnownFormat.store(true);
    }

    if (!m_HasLastKnownFormat.load()) return;

    const AudioFormat& format = m_LastKnownFormat;

    size_t targetSamples = static_cast<size_t>(realIntervalSec * 
        static_cast<double>(format.SamplesPerSec) * 
        static_cast<double>(format.Channels));

    targetSamples = (targetSamples / format.Channels) * format.Channels;

    if (targetSamples == 0) return;

    if (isMuted)
    {
        // Discard data from all buffers and send zeroed chunks to the encoder.
        for (auto& inst : m_ActiveInstances)
        {
            size_t toDrain = (std::min)(inst.PendingSamples.size(), targetSamples);

            if (toDrain > 0)
            {
                inst.PendingSamples.erase(inst.PendingSamples.begin(),
                    inst.PendingSamples.begin() + toDrain);
            }
        }

        this->EmitSilenceChunk(format, targetSamples);

        m_SilenceFallbackAccumSec.store(0.0);
        return;
    }

    if (currentInstance != nullptr)
    {
        if (currentInstance->Instance != m_LastSelectedInstance.load())
        {
            m_LastSelectedInstance.store(currentInstance->Instance);
        }

        m_MixBuffer.assign(targetSamples, 0.0f);
        const float* src = currentInstance->PendingSamples.data();

        for (size_t i = 0; i < targetSamples; ++i)
        {
            m_MixBuffer[i] = src[i];
        }

        float maxSample = 0.0f;

        for (float s : m_MixBuffer)
        {
            if (std::abs(s) > maxSample) maxSample = std::abs(s);
        }

        if (maxSample > 1.0f)
        {
            float scale = 1.0f / maxSample;
            for (float& s : m_MixBuffer) s *= scale;
        }

        for (auto& inst : m_ActiveInstances)
        {
            size_t toDrain = (std::min)(inst.PendingSamples.size(), targetSamples);

            if (toDrain > 0)
            {
                inst.PendingSamples.erase(inst.PendingSamples.begin(),
                    inst.PendingSamples.begin() + toDrain);
            }
        }

        if (m_SilenceFallbackAccumSec.load() > 0.0)
        {
            g_pSystem->Debug->Log("[AudioSystem] INFO: Audio signal recovered after"
                " %.2fs of silence fallback.", m_SilenceFallbackAccumSec.load());
        }

        m_SilenceFallbackAccumSec.store(0.0);

        uint64_t samplesBeforeMix = m_TotalSamplesMixed.load();
        m_TotalSamplesMixed += targetSamples;

        AudioChunk chunk;
        chunk.DurationSec = static_cast<double>(m_TotalSamplesMixed.load()) / 
            (format.SamplesPerSec * format.Channels) - 
            static_cast<double>(samplesBeforeMix) / 
            (format.SamplesPerSec * format.Channels);

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

    // If no instance had enough data, clear what's available and emit silence to maintain sync.
    for (auto& inst : m_ActiveInstances)
    {
        size_t toDrain = (std::min)(inst.PendingSamples.size(), targetSamples);

        if (toDrain > 0)
        {
            inst.PendingSamples.erase(inst.PendingSamples.begin(),
                inst.PendingSamples.begin() + toDrain);
        }
    }

    m_SilenceFallbackAccumSec += realIntervalSec;

    if (m_SilenceFallbackAccumSec.load() > k_MaxSilenceFallbackSec) return;

    if (m_SilenceFallbackAccumSec.load() < 0.1)
    {
        g_pSystem->Debug->Log("[AudioSystem] INFO: No active instance with signal,"
            " emitting silence fallback (limit=%.0fs).", k_MaxSilenceFallbackSec);
    }

    this->EmitSilenceChunk(format, targetSamples);
}


void AudioSystem::EmitSilenceChunk(const AudioFormat& fmt, size_t targetSamples)
{
    uint64_t samplesBeforeMix = m_TotalSamplesMixed.load();
    m_TotalSamplesMixed += targetSamples;

    AudioChunk chunk;
    chunk.DurationSec = static_cast<double>(m_TotalSamplesMixed.load()) / 
        (fmt.SamplesPerSec * fmt.Channels) - 
        static_cast<double>(samplesBeforeMix) / 
        (fmt.SamplesPerSec * fmt.Channels);

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
    std::lock_guard<std::mutex> lock(m_ActiveInstancesMutex);

    auto now = std::chrono::steady_clock::now();
    std::vector<void*> instancesToRemove;

    for (const auto& activeInstace : m_ActiveInstances)
    {
        if ((now - activeInstace.LastDataTime) > m_InactiveTimeout)
        {
            instancesToRemove.push_back(activeInstace.Instance);
        }
    }

    for (void* instance : instancesToRemove)
    {
        g_pSystem->Debug->Log("[AudioSystem] INFO: Removing stale instance.");
        g_pState->Infrastructure->Audio->UnregisterAudioInstance(instance);
    }

    m_ActiveInstances.erase(
        std::remove_if(m_ActiveInstances.begin(), m_ActiveInstances.end(),
            [now](const ActiveInstance& inst) {
                return (now - inst.LastDataTime) > m_InactiveTimeout;
            }),
        m_ActiveInstances.end());
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