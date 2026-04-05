#pragma once

#include "Core/Common/Types/FFmpegTypes.h"
#include <condition_variable>
#include <cstdint>
#include <vector>
#include <mutex>
#include <deque>

class WriterThread
{
public:
	void Run();

	void StartRecording();
	void StopRecording(bool force = false);

	void EnqueueVideo(std::vector<uint8_t>&& buffer, bool returnToPool = true);
	void EnqueueAudio(std::vector<uint8_t>&& buffer);
	void Flush();

	void DropOldestVideo();

	void Cleanup();

private:
	std::deque<Item> m_Queue;
	mutable std::mutex m_Mutex;

	std::condition_variable m_CV;
	std::condition_variable m_CV_Flush;

	std::atomic<bool> m_Running{ true };
	std::atomic<bool> m_Recording{ false };
	std::atomic<bool> m_IsProcessing{ false };

};