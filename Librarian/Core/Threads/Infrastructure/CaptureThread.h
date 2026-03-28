#pragma once

#include "Core/Common/Types/VideoTypes.h"
#include "Core/Common/Types/AudioTypes.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>

class CaptureThread
{
public:
	void Run();

	void StartRecording();
	void StopRecording(bool force = false);

	size_t GetPendingAudioSize();
	size_t GetPendingVideoSize();

	double GetSyncRatio() const;

private:
	void VerifyAndPrepareFFmpeg();
	bool ReadyToCapture();

	bool VideoQueueOverflow(size_t pendingSize);

	bool CaptureBaselineEstablished(std::deque<AudioChunk>& audioQueue, std::deque<FrameData>& videoQueue);
	void ProcessSynchronizedStreams(std::deque<AudioChunk>& audioQueue, std::deque<FrameData>& videoQueue, bool forceDrain);

	void Cleanup();

	std::vector<BYTE> m_LastFrameBuffer{ 0 };

	std::atomic<bool> m_SyncTimeInitialized{ false };
	std::atomic<bool> m_SyncInitialized{ false };
	std::atomic<bool> m_StopByForce{ false };

	std::deque<AudioChunk> m_PendingAudio{};
	std::deque<FrameData> m_PendingVideo{};
	std::mutex m_PendingMutex{};

	std::chrono::steady_clock::time_point m_LastStatLog{};
	uint64_t m_TotalVideoFramesWritten{ 0 };
	uint64_t m_TotalAudioSamplesWritten{ 0 };
	double m_TotalAudioDuration = 0.0;
	std::atomic<double> m_SyncRatio{ 0.0 };

	const int m_MaxItemsPerCycle = 150;
};