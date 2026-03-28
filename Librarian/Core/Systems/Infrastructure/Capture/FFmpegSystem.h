#pragma once

#include "Core/Common/Types/FFmpegTypes.h"
#include <string>
#include <atomic>

class FFmpegSystem
{
public:
	bool Start(const std::string& outputPath, int width, int height, float fps);
	void ForceStop();
	void Stop();

	void InitializeDependencies();

	bool WriteVideo(void* data, size_t size);
	bool WriteAudio(const void* data, size_t size);

	float GetRecordingDuration() const;
	bool HasFatalError() const;

	CaptureTelemetry GetTelemetry() const;
	void UpdateQueueTelemetry();

	void Cleanup();

private:
	bool CreatePipes(std::string& videoPipeName, std::string& audioPipeName, int width, int height);
	std::string BuildFFmpegCommand(std::string outputPath, int width, int height, float fps, std::string videoPipeName, std::string audioPipeName);
	std::string GenerateTimestampName();
	bool LaunchFFmpeg(const std::string& cmd);

	void InternalStop(bool force);
	bool VerifyExecutable(const std::string& path);

	bool WriteWithTimeout(HANDLE hPipe, const void* data, size_t size, DWORD timeoutMs, bool isVideo);

	void ReadLogsThread(HANDLE hPipe);
	void TranslateAndLog(const std::string& logLine);

	uint32_t m_SessionID = 0;

	std::atomic<bool> m_FFmpegReportedError{ false };
	std::atomic<HANDLE> m_hLogRead{ NULL };
	std::atomic<HANDLE> m_hLogWrite{ NULL };

	std::atomic<bool> m_VideoConnected{ false };
	std::atomic<bool> m_AudioConnected{ false };

	std::chrono::steady_clock::time_point m_LastVideoWriteTime{};
	std::chrono::steady_clock::time_point m_LastAudioWriteTime{};

	int m_ConsecutiveWriteFailures = 0;
	const int m_MaxConsecutiveWriteFailures = 5;
	static constexpr double k_PipeDeadSec = 5.0;

	CaptureTelemetry m_Telemetry;
	mutable std::mutex m_TelemetryMutex;
};