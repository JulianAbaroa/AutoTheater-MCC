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

	void Cleanup();

private:
	void VerifyFFmpeg();
	bool IsReady();

	bool CheckFramerate(int framerate);

	bool VideoQueueOverflow(size_t pendingSize);

	bool CaptureBaselineEstablished();
	void ClearPendingResources();

	void ProcessSynchronizedStreams(bool forceDrain);


	std::vector<BYTE> m_LastFrameBuffer{};

	double m_SyncRatio = 0.0;
	bool m_SyncInitialized = false;
	std::atomic<bool> m_StopByForce{ false };
	std::atomic<bool> m_StopInProgress{ false };

	std::deque<AudioChunk> m_PendingAudio{};
	std::deque<FrameData> m_PendingVideo{};

	uint64_t m_VideoFramesWritten{ 0 };
	double m_AudioDuration = 0.0;

	size_t m_DiagnosticCounter = 0;

	std::atomic<int> m_CaptureMaxFrames{ 0 };
	std::atomic<int> m_WriterMaxFrames{ 0 };
};