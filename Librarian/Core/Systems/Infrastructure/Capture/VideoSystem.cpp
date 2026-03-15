#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/VideoState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"
#include "Core/Systems/Infrastructure/Capture/SyncSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

void VideoSystem::StartRecording()
{
    this->ClearQueue();
	g_pState->Infrastructure->Video->SetRecording(true);
    g_pSystem->Debug->Log("[VideoSystem] INFO: Video recording started.");
}

void VideoSystem::StopRecording()
{
    this->ClearQueue();
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

size_t VideoSystem::GetQueueSize()
{
    std::lock_guard<std::mutex> lock(m_QueueMutex);
    if (m_FrameQueue.empty()) return 0;
    return m_FrameQueue.size();
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
    std::scoped_lock lock(m_PoolMutex, m_QueueMutex);

    size_t newBufferSize = static_cast<size_t>(width) * height * 4;

    if (newBufferSize == m_BufferSize && m_FreeBuffers.size() == m_MaxPoolSize)
    {
        g_pSystem->Debug->Log("[VideoSystem] INFO: Pool already allocated with correct size, skipping.");
        return;
    }

    m_FreeBuffers.clear();
    m_BufferSize = newBufferSize;

    for (int i = 0; i < m_MaxPoolSize; i++)
    {
        std::vector<uint8_t> buf;
        buf.resize(m_BufferSize);
        m_FreeBuffers.push_back(std::move(buf));
    }

    g_pSystem->Debug->Log("[VideoSystem] INFO: Pool reallocated (%zu MB per buffer)", m_BufferSize / (1024 * 1024));
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

    std::vector<uint8_t> bufferToUse;
    {
        std::lock_guard<std::mutex> lock(m_PoolMutex);
        if (!m_FreeBuffers.empty())
        {
            bufferToUse = std::move(m_FreeBuffers.front());
            m_FreeBuffers.pop_front();
        }
    }

    if (bufferToUse.empty()) bufferToUse.resize(m_BufferSize);

    size_t targetStride = static_cast<size_t>(width) * 4;
    if (rowPitch == targetStride)
    {
        memcpy(bufferToUse.data(), pData, m_BufferSize);
    }
    else
    {
        for (size_t i = 0; i < height; ++i)
        {
            memcpy(bufferToUse.data() + (i * targetStride), pData + (i * rowPitch), targetStride);
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
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

    g_pState->Infrastructure->Video->Cleanup();

    g_pSystem->Debug->Log("[VideoSystem] INFO: Video cleanup completed.");
}