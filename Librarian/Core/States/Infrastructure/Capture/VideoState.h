#pragma once

#include "Core/Common/Types/VideoTypes.h"
#include <atomic>
#include <queue>
#include <mutex>

class VideoState
{
public:
    bool IsRecording() const;
    void SetRecording(bool value);

	int GetMaxFrames() const;
	void SetMaxFrames(int maxFrames);

	size_t GetFrameByteSize() const;
	void SetFrameByteSize(size_t size);

	void SwapFrameQueue(std::deque<FrameData>& outQueue);
	std::deque<FrameData> DiscardAndTakeQueue();

	void InitializePool(size_t frameSize, int maxPoolSize, 
		std::deque<std::vector<uint8_t>>&& initialBuffers);
	bool IsPoolValid(size_t requiredSize) const;
	int GetMaxPoolSize() const;
	void ClearPool();

	void PushFreeBuffer(std::vector<uint8_t>&& buffer);
	std::vector<uint8_t> PopFreeBuffer();
	size_t GetTotalAllocatedBuffers() const;

	bool TryPushFrame(FrameData&& frame);

	void Reset();
    void Cleanup();

private:
    std::atomic<bool> m_IsRecording{ false };

	std::deque<FrameData> m_FrameQueue;
	mutable std::mutex m_QueueMutex;

	std::deque<std::vector<uint8_t>> m_FreeBuffers;
	mutable std::mutex m_PoolMutex;

	int m_MaxFrames = 0;
	int m_MaxPoolSize = 0;
	size_t m_FrameByteSize = 0;
};