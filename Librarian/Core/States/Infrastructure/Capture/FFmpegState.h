#pragma once

#include "Core/Common/Types/FFmpegTypes.h"
#include <string>
#include <atomic>
#include <mutex>

struct FFmpegState 
{
public:
	bool RecordingStarted() const;
	void SetStartRecording(bool value);

	bool RecordingStopped() const;
	void SetStopRecording(bool value);

	bool IsRecording() const;
	void SetRecording(bool value);

	bool IsCaptureActive() const;
	void SetCaptureActive(bool value);

	std::chrono::steady_clock::time_point GetStartRecordingTime() const;
	void SetStartRecordingTime(std::chrono::steady_clock::time_point time_point);
	
	HANDLE GetVideoPipeHandle() const;
	void SetVideoPipeHandle(HANDLE h);

	HANDLE GetAudioPipeHandle() const;
	void SetAudioPipeHandle(HANDLE h);

	HANDLE GetProcessHandle() const;
	void SetProcessHandle(HANDLE h);

	ResolutionType GetResolutionType();
	void SetResolutionType(ResolutionType type);

	int GetTargetWidth();
	int GetTargetHeight();

	float GetTargetFramerate() const;
	void SetTargetFramerate(float framerate);

	bool ShouldRecordUI() const;
	void SetRecordUI(bool value);

	std::string GetOutputPath() const;
	void SetOutputPath(std::string outputPath);

	bool StopOnLastEvent() const;
	void SetStopOnLastEvent(bool value);

	float GetStopDelayDuration() const;
	void SetStopDelayDuration(float value);

	bool IsFFmpegInstalled() const;
	void SetFFmpegInstalled(bool value);

	bool IsDownloadInProgress() const;
	void SetDownloadInProgress(bool value);

	float GetDownloadProgress() const;
	void SetDownloadProgress(float value);

	std::string GetExecutablePath() const;
	void SetExecutablePath(std::string path);

	void Cleanup();

	FFmpegEncoderConfig GetEncoderConfig() const;
	void UpdateEncoderConfig(const FFmpegEncoderConfig newEncoderConfig);

private:
	std::atomic<bool> m_StartRecording{ false };
	std::atomic<bool> m_StopRecording{ false };
	std::atomic<bool> m_IsRecording{ false };
	std::atomic<bool> m_IsCaptureActive{ false };
	
	std::atomic<HANDLE> m_hVideoPipe{ INVALID_HANDLE_VALUE };
	std::atomic<HANDLE> m_hAudioPipe{ INVALID_HANDLE_VALUE };
	std::atomic<HANDLE> m_hProcess{ INVALID_HANDLE_VALUE };

	std::chrono::steady_clock::time_point m_StartRecordingTime;
	std::atomic<ResolutionType> m_ResolutionType{ ResolutionType::UHD_4K };
	std::atomic<float> m_TargetFramerate{ 60.0f };
	std::atomic<bool> m_ShouldRecordUI{ false };
	std::atomic<bool> m_StopOnLastEvent{ false };
	std::atomic<float> m_StopDelayDuration{ 0.0f };

	std::string m_OutputPath{};

	std::atomic<bool> m_IsFFmpegInstalled{ false };
	std::atomic<bool> m_IsDownloadInProgress{ false };
	std::atomic<float> m_DownloadProgress{ 0.0f }; 
	std::string m_ExecutablePath;
	mutable std::mutex m_Mutex;

	FFmpegEncoderConfig m_EncoderConfig;
	mutable std::mutex m_EncoderConfigMutex;
};