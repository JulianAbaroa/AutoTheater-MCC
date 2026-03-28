#pragma once

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

	size_t GetPendingSize() const;

	void DropOldestVideo();

private:
	enum class ItemType { Video, Audio };

	struct Item
	{
		ItemType Type;
		std::vector<uint8_t> Data;
		bool ReturnVideoToPool = false;
	};

	std::deque<Item> m_Queue;
	mutable std::mutex m_Mutex;
	std::condition_variable m_CV;

	std::atomic<bool> m_Running{ true };
	std::atomic<bool> m_Recording{ false };
	std::atomic<bool> m_StopByForce{ false };
};