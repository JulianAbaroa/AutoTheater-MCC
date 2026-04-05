#include "pch.h"
#include "Core/States/Infrastructure/Capture/VideoState.h"

bool VideoState::IsRecording() const { return m_IsRecording.load(); }
void VideoState::SetRecording(bool value) { m_IsRecording.store(value); }

int VideoState::GetMaxFrames() const { return m_MaxFrames; }
void VideoState::SetMaxFrames(int maxFrames) { m_MaxFrames = maxFrames; }

size_t VideoState::GetFrameByteSize() const { return m_FrameByteSize; }
void VideoState::SetFrameByteSize(size_t size) { m_FrameByteSize = size; }

void VideoState::SwapFrameQueue(std::deque<FrameData>& outQueue) 
{
	std::lock_guard<std::mutex> lock(m_QueueMutex);
	outQueue.swap(m_FrameQueue);
}

std::deque<FrameData> VideoState::DiscardAndTakeQueue()
{
	std::deque<FrameData> temporal;
	std::lock_guard<std::mutex> lock(m_QueueMutex);
	temporal.swap(m_FrameQueue);
	return temporal;
}

void VideoState::InitializePool(size_t frameSize, int maxPoolSize,
	std::deque<std::vector<uint8_t>>&& initialBuffers)
{
	std::scoped_lock lock(m_PoolMutex, m_QueueMutex);
	m_FrameByteSize = frameSize;
	m_MaxPoolSize = maxPoolSize;
	m_FreeBuffers = std::move(initialBuffers);
}

bool VideoState::IsPoolValid(size_t requiredSize) const
{
	std::lock_guard<std::mutex> lock(m_PoolMutex);
	return (m_FrameByteSize == requiredSize && m_FreeBuffers.size() >= 8);
}

int VideoState::GetMaxPoolSize() const { return m_MaxPoolSize; }

void VideoState::ClearPool()
{
	std::lock_guard<std::mutex> lock(m_PoolMutex);
	m_FreeBuffers.clear();
}

void VideoState::PushFreeBuffer(std::vector<uint8_t>&& buffer)
{
	if (buffer.empty()) return;

	std::lock_guard<std::mutex> lock(m_PoolMutex);

	if (buffer.size() != m_FrameByteSize) return;

	if (m_FreeBuffers.size() < (size_t)m_MaxPoolSize)
	{
		m_FreeBuffers.push_back(std::move(buffer));
	}
}

std::vector<uint8_t> VideoState::PopFreeBuffer()
{
	std::lock_guard<std::mutex> lock(m_PoolMutex);
	if (m_FreeBuffers.empty()) return {};

	std::vector<uint8_t> buffer = std::move(m_FreeBuffers.back());
	m_FreeBuffers.pop_back();
	return buffer;
}

size_t VideoState::GetTotalAllocatedBuffers() const
{
	std::scoped_lock lock(m_PoolMutex, m_QueueMutex);
	return m_FreeBuffers.size() + m_FrameQueue.size();
}

bool VideoState::TryPushFrame(FrameData&& frame)
{
	std::lock_guard<std::mutex> lock(m_QueueMutex);

	if (m_FrameQueue.size() >= (size_t)m_MaxFrames)
	{
		this->PushFreeBuffer(std::move(frame.Buffer));
		return false;
	}

	m_FrameQueue.push_back(std::move(frame));
	return true;
}

void VideoState::Reset()
{
	std::scoped_lock lock(m_QueueMutex, m_PoolMutex);

	while (!m_FrameQueue.empty())
	{
		std::vector<uint8_t> buffer = std::move(m_FrameQueue.front().Buffer);

		if (buffer.size() == m_FrameByteSize && 
			m_FreeBuffers.size() < (size_t)m_MaxPoolSize)
		{
			m_FreeBuffers.push_back(std::move(buffer));
		}

		m_FrameQueue.pop_front();
	}
}

void VideoState::Cleanup() 
{ 
	std::scoped_lock lock(m_QueueMutex, m_PoolMutex);

	m_FrameQueue.clear();
	m_FreeBuffers.clear();

	m_MaxFrames = 0;
	m_MaxPoolSize = 0;
	m_FrameByteSize = 0;
	m_IsRecording.store(false); 
}