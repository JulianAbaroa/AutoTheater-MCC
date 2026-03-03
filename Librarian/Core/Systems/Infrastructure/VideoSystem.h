#pragma once

#include "Core/Common/Types/VideoTypes.h"
#include <queue>
#include <mutex>

class VideoSystem
{
public:
	void StartRecording();
	void StopRecording();

	void PushFrame(const uint8_t* pData, UINT width, UINT height, UINT rowPitch, float engineTime);
	
	std::deque<FrameData> ExtractQueue();
	size_t GetQueueSize();
	void ClearQueue();

	void Cleanup();

private:
	std::deque<FrameData> m_FrameQueue;
	std::mutex m_QueueMutex;
};