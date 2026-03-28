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

	int GetPoolSize() const;
	int GetPoolMaxSize() const;
	size_t GetBufferSize() const;

	uint64_t GetPoolTaken() const;
	uint64_t GetPoolReturned() const;
	uint64_t GetPoolDiscarded() const;

	void Cleanup();

private:
	std::deque<FrameData> m_FrameQueue;
	std::mutex m_QueueMutex;

	std::deque<std::vector<uint8_t>> m_FreeBuffers;
	mutable std::mutex m_PoolMutex;

	int m_MaxPoolSize = 120;
	size_t m_BufferSize = 0;

	size_t m_CachedBufferSize{};
	int m_MaxFrames = 0;

	std::atomic<uint64_t> m_PoolTaken{ 0 };
	std::atomic<uint64_t> m_PoolReturned{ 0 };
	std::atomic<uint64_t> m_PoolDiscarded{ 0 };
};