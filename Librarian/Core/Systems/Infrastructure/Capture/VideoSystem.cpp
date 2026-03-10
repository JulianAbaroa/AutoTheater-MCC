#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/VideoState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"

void VideoSystem::StartRecording()
{
    this->ClearQueue();
	g_pState->Infrastructure->Video->SetRecording(true);
    g_pUtil->Log.Append("[VideoSystem] INFO: Frames recording started.");
}

void VideoSystem::StopRecording()
{
    this->ClearQueue();
	g_pState->Infrastructure->Video->SetRecording(false);
    g_pUtil->Log.Append("[VideoSystem] INFO: Frames recording stopped.");
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
    if (m_FrameQueue.empty()) return;
    m_FrameQueue.clear();
}


void VideoSystem::PreallocatePool(UINT width, UINT height) 
{
    std::lock_guard<std::mutex> lock(m_PoolMutex);
    m_BufferSize = static_cast<size_t>(width) * height * 4;

    m_FreeBuffers.clear();
    for (int i = 0; i < m_MaxPoolSize; i++) 
    {
        m_FreeBuffers.emplace_back(m_BufferSize);
    }
}

void VideoSystem::ReturnBuffer(std::vector<uint8_t>&& buffer) 
{
    std::lock_guard<std::mutex> lock(m_PoolMutex);
    if (m_FreeBuffers.size() < m_MaxPoolSize) 
    {
        m_FreeBuffers.push_back(std::move(buffer));
    }
}

void VideoSystem::PushFrame(const uint8_t* pData, UINT width, UINT height, UINT rowPitch, float engineTime) 
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
        frame.engineTime = engineTime;
        frame.buffer = std::move(bufferToUse);
        m_FrameQueue.push_back(std::move(frame));
    }
}