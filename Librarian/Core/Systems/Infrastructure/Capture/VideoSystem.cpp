#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/VideoState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"
#include "Core/Systems/Infrastructure/Capture/SyncSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

void VideoSystem::StartRecording()
{
    this->ClearQueue();

    auto config = g_pState->Infrastructure->FFmpeg->GetEncoderConfig();
    m_MaxFrames = config.MaxBufferedFrames;

    int inW = g_pState->Infrastructure->Render->GetWidth() & ~1;
    int inH = g_pState->Infrastructure->Render->GetHeight() & ~1;
    m_CachedBufferSize = static_cast<size_t>(inW & ~1) * (inH & ~1) * 4;

	g_pState->Infrastructure->Video->SetRecording(true);
    g_pSystem->Debug->Log("[VideoSystem] INFO: Video recording started.");
}

void VideoSystem::StopRecording()
{
    this->ClearQueue();
    this->Cleanup();
	g_pState->Infrastructure->Video->SetRecording(false);
    g_pSystem->Debug->Log("[VideoSystem] INFO: Video recording stopped.");
}


std::deque<FrameData> VideoSystem::ExtractQueue()
{
	std::deque<FrameData> localQueue;
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		if (m_FrameQueue.empty()) return localQueue;
		localQueue.swap(m_FrameQueue);
	}

	return localQueue;
}

void VideoSystem::ClearQueue()
{
    std::lock_guard<std::mutex> lock(m_QueueMutex);
    while (!m_FrameQueue.empty())
    {
        this->ReturnBuffer(std::move(m_FrameQueue.front().Buffer));
        m_FrameQueue.pop_front();
    }

    g_pSystem->Debug->Log("[VideoSystem] INFO: Video queue cleared.");
}


void VideoSystem::PreallocatePool(UINT width, UINT height)
{
    size_t newBufferSize = static_cast<size_t>(width) * height * 4;
    auto encoderConfig = g_pState->Infrastructure->FFmpeg->GetEncoderConfig();
    int maxFrames = encoderConfig.MaxBufferedFrames;

    {
        std::scoped_lock lock(m_PoolMutex, m_QueueMutex);
        if (newBufferSize == m_BufferSize && (int)m_FreeBuffers.size() >= 8)
        {
            g_pSystem->Debug->Log("[VideoSystem] INFO: Pool already allocated with correct size, skipping.");
            return;
        }
        m_FreeBuffers.clear();
        m_BufferSize = newBufferSize;
        m_MaxPoolSize = maxFrames;
    }

    std::deque<std::vector<uint8_t>> initialBuffers;
    for (int i = 0; i < 8; i++)
    {
        std::vector<uint8_t> buf(newBufferSize, 0);
        initialBuffers.push_back(std::move(buf));
    }

    {
        std::scoped_lock lock(m_PoolMutex, m_QueueMutex);
        m_FreeBuffers = std::move(initialBuffers);
    }

    g_pSystem->Debug->Log("[VideoSystem] INFO: Pool initialized with 8 frames. Will grow up to %d. FrameSize: %zu MB",
        maxFrames, newBufferSize / (1024ULL * 1024));
}

void VideoSystem::ReturnBuffer(std::vector<uint8_t>&& buffer)
{
    std::lock_guard<std::mutex> lock(m_PoolMutex);

    if (buffer.size() != m_BufferSize)
    {
        return;
    }

    if (m_FreeBuffers.size() < m_MaxPoolSize)
    {
        m_FreeBuffers.push_back(std::move(buffer));
    }
}

void VideoSystem::PushFrame(const uint8_t* pData, UINT width, UINT height, UINT rowPitch, double engineTime)
{
    if (!g_pState->Infrastructure->Video->IsRecording()) return;

    UINT w = width & ~1;
    UINT h = height & ~1;

    std::vector<uint8_t> bufferToUse;

    {
        std::lock_guard<std::mutex> lock(m_PoolMutex);
        if (!m_FreeBuffers.empty())
        {
            bufferToUse = std::move(m_FreeBuffers.front());
            m_FreeBuffers.pop_front();
        }
    }

    if (bufferToUse.size() != m_CachedBufferSize)
    {
        bufferToUse.resize(m_CachedBufferSize);
    }

    size_t targetStride = static_cast<size_t>(w) * 4;
    if (rowPitch == targetStride)
    {
        memcpy(bufferToUse.data(), pData, m_CachedBufferSize);
    }
    else
    {
        for (UINT i = 0; i < h; ++i)
        {
            memcpy(bufferToUse.data() + (i * targetStride), pData + (i * rowPitch), targetStride);
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);

        if (m_FrameQueue.size() > m_MaxFrames)
        {
            std::lock_guard<std::mutex> poolLock(m_PoolMutex);
            m_FreeBuffers.push_back(std::move(bufferToUse));
            g_pSystem->Debug->Log("[VideoSystem] WARNING: Frame dropped! FrameQueue is full.");
            return;
        }

        FrameData frame;
        frame.RealTime = engineTime;
        frame.Buffer = std::move(bufferToUse);
        m_FrameQueue.push_back(std::move(frame));
    }
}


void VideoSystem::Cleanup()
{
    {
        std::lock_guard<std::mutex> queueLock(m_QueueMutex);
        m_FrameQueue.clear();
    }

    {
        std::lock_guard<std::mutex> pollLock(m_PoolMutex);
        m_FreeBuffers.clear();
    }

    m_BufferSize = 0;
    m_MaxFrames = 60;
    m_CachedBufferSize = {};

    g_pState->Infrastructure->Video->Cleanup();

    g_pSystem->Debug->Log("[VideoSystem] INFO: Cleanup completed.");
}