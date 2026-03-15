#pragma once

#include "Core/Common/Types/VideoTypes.h"
#include "Core/Common/Types/AudioTypes.h"
#include <atomic>
#include <chrono>
#include <deque>

class CaptureThread
{
public:
	void Run();

private:
	void VerifyAndPrepareFFmpeg();
	bool ReadyToCapture();
	
	void StartRecording();
	void StopRecording();

	bool VideoQueueOverflow(size_t pendingSize);
	void MonitorRecordingHealth();

	bool CaptureBaselineEstablished(std::deque<AudioChunk>& audioQueue, std::deque<FrameData>& videoQueue);
	void ProcessSynchronizedStreams(std::deque<AudioChunk>& audioQueue, std::deque<FrameData>& videoQueue, bool forceDrain);

	void Cleanup();

	std::atomic<uint64_t> m_TotalVideoFramesWritten{ 0 };
	std::atomic<uint64_t> m_TotalAudioSamplesWritten{ 0 };
	std::atomic<bool> m_SyncTimeInitialized{ false };
	std::atomic<bool> m_SyncInitialized{ false };
	std::vector<BYTE> m_LastFrameBuffer{ 0 };
	AudioFormat m_MasterFormat{};
};