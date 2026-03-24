#pragma once

#include "Core/Common/Types/VideoTypes.h"
#include <queue>
#include <mutex>

class VideoSystem
{
public:
	void StartRecording();
	void StopRecording();

	void PushFrame(const uint8_t* pData, UINT width, UINT height, UINT rowPitch, double engineTime);
	
	std::deque<FrameData> ExtractQueue();
	void ClearQueue();

	void PreallocatePool(UINT width, UINT height);
	void ReturnBuffer(std::vector<uint8_t>&& buffer);

	void Cleanup();

private:
	std::deque<FrameData> m_FrameQueue;
	std::mutex m_QueueMutex;

	std::deque<std::vector<uint8_t>> m_FreeBuffers;
	std::mutex m_PoolMutex;

	const size_t m_MaxPoolSize = 64;
	size_t m_BufferSize = 0;
};