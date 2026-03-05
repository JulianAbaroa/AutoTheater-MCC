#pragma once

#include <string>
#include <atomic>

class FFmpegSystem
{
public:
	bool Start(const std::string& outputPath, int width, int height, float fps);
	void ForceStop();
	void Stop();

	void WriteVideo(void* data, size_t size);
	void WriteAudio(const void* data, size_t size);
	bool WriteWithTimeout(HANDLE hPipe, const void* data, size_t size, DWORD timeoutMs);
	
	void InitializeFFmpeg();
	bool VerifyExecutable(const std::string& path);

	bool DownloadFFmpeg();
	void CancelDownload();
	bool UninstallFFmpeg();
	
	std::string GenerateTimestampName();

	float GetRecordingDuration() const;

	bool IsAudioConnected();
	void Cleanup();

private:
	std::string BuildFFmpegCommand(std::string outputPath, int width, int height, float fps, std::string videoPipeName, std::string audioPipeName);
	void InternalStop(bool force);

	bool LaunchFFmpeg(const std::string& cmd);
	bool CreatePipes(std::string& videoPipeName, std::string& audioPipeName);

	void ReadLogsThread(HANDLE hPipe);

	std::atomic<uint32_t> m_SessionID{ 0 };
	std::atomic<bool> m_FFmpegReportedError{ false };
	std::atomic<HANDLE> m_hLogRead{ NULL };
	std::atomic<HANDLE> m_hLogWrite{ NULL };
	std::atomic<bool> m_VideoConnected{ false };
	std::atomic<bool> m_AudioConnected{ false };
};