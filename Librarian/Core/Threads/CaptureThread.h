#pragma once

#include "Core/Common/Types/VideoTypes.h"
#include <atomic>
#include <chrono>

class CaptureThread
{
public:
	void Run();

private:
	bool CaptureBaselineEstablished(std::deque<AudioChunk>& audioQueue, std::deque<FrameData>& videoQueue);
	void ProcessSynchronizedStreams(std::deque<AudioChunk>& audioQueue, std::deque<FrameData>& videoQueue);
	void MonitorRecordingHealth();
	void VerifyAndPrepareFFmpeg();
	bool VideoQueueOverflow();
	bool ReadyToCapture();
	void StartRecording();
	void StopRecording();

	std::chrono::steady_clock::time_point m_RecordingStartTime{};
	std::atomic<uint64_t> m_TotalVideoFramesWritten{ 0 };
	std::atomic<uint64_t> m_TotalAudioSamplesWritten{ 0 };
	std::atomic<bool> m_SyncTimeInitialized{ false };
	std::atomic<bool> m_SyncInitialized{ false };
	std::vector<BYTE> m_LastFrameBuffer{ 0 };
	AudioFormat m_MasterFormat{};
};