#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"

void VideoSystem::StartRecording()
{
    this->Cleanup();
	g_pState->Video.SetRecording(true);
    g_pUtil->Log.Append("[VideoSystem] INFO: Frames recording started.");
}

void VideoSystem::StopRecording()
{
    this->Cleanup();
	g_pState->Video.SetRecording(false);
    g_pUtil->Log.Append("[VideoSystem] INFO: Frames recording stopped.");
}

void VideoSystem::Cleanup()
{
    this->ClearQueue();
    g_pUtil->Log.Append("[VideoSystem] INFO: Cleanup and memory freed.");
}


void VideoSystem::PushFrame(const uint8_t* pData, UINT width, UINT height, UINT rowPitch, float engineTime)
{
    if (!g_pState->Video.IsRecording()) return;

    size_t targetStride = static_cast<size_t>(width) * 4;
    size_t totalSize = targetStride * height;

    FrameData frame;
    frame.engineTime = engineTime;
    frame.buffer.resize(totalSize);

    if (rowPitch == targetStride) 
    {
        memcpy(frame.buffer.data(), pData, totalSize);
    }
    else 
    {
        for (size_t i = 0; i < height; ++i) 
        {
            memcpy(frame.buffer.data() + (i * targetStride), pData + (i * rowPitch), targetStride);
        }
    }

    // Protect RAM
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);

        if (m_FrameQueue.size() >= 64)
        {
            g_pUtil->Log.Append("[VideoSystem] Frame queue full, dropping this frame.");
            m_FrameQueue.pop_front();
        }

        m_FrameQueue.push_back(std::move(frame));
    }
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