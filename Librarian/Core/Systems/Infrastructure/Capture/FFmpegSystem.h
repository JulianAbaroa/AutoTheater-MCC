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
	bool VerifyExecutable(const std::string& path);

	void TranslateAndLog(const std::string& logLine);
	std::string GenerateTimestampName();
	float GetRecordingDuration() const;

	bool WriteVideo(void* data, size_t size);
	bool WriteAudio(const void* data, size_t size);
	bool WriteWithTimeout(HANDLE hPipe, const void* data, size_t size, DWORD timeoutMs);

	void Cleanup();

private:
	std::string BuildFFmpegCommand(std::string outputPath, int width, int height, float fps, std::string videoPipeName, std::string audioPipeName);
	void InternalStop(bool force);

	bool LaunchFFmpeg(const std::string& cmd);
	void ReadLogsThread(HANDLE hPipe);

	bool CreatePipes(std::string& videoPipeName, std::string& audioPipeName);

	std::atomic<uint32_t> m_SessionID{ 0 };

	std::atomic<bool> m_FFmpegReportedError{ false };
	std::atomic<HANDLE> m_hLogRead{ NULL };
	std::atomic<HANDLE> m_hLogWrite{ NULL };

	std::atomic<bool> m_VideoConnected{ false };
	std::atomic<bool> m_AudioConnected{ false };

	std::atomic<int> m_CurrentInW{ 0 };
	std::atomic<int> m_CurrentInH{ 0 };
	std::atomic<int> m_CurrentOutW{ 0 };
	std::atomic<int> m_CurrentOutH{ 0 };

	FFmpegEncoderConfig m_CurrentEncoderConfig{};

	std::chrono::steady_clock::time_point m_LastVideoWriteTime;
	std::chrono::steady_clock::time_point m_LastAudioWriteTime;
	int m_VideoStallCount = 0;
	int m_AudioStallCount = 0;
	const int m_MaxStallCount = 3;
	bool m_InRecoveryMode = false;
};