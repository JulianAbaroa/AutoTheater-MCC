#pragma once

#include "Core/Common/Types/FFmpegTypes.h"
#include <string>
#include <atomic>
#include <mutex>

class FFmpegState 
{
public:
	using TimePoint = std::chrono::steady_clock::time_point;

	// Recording state.
	bool RecordingStarted() const;
	bool RecordingStopped() const;
	bool IsRecording() const;
	bool IsCaptureActive() const;
	float GetRecordingSpeed() const;
	TimePoint GetStartRecordingTime() const;

	void SetStartRecording(bool value);
	void SetStopRecording(bool value);
	void SetRecording(bool value);
	void SetCaptureActive(bool value);
	void SetRecordingSpeed(float value);
	void SetStartRecordingTime(TimePoint value);

	// Handles.
	HANDLE GetVideoPipeHandle() const;
	HANDLE GetAudioPipeHandle() const;
	HANDLE GetProcessHandle() const;

	void SetVideoPipeHandle(HANDLE h);
	void SetAudioPipeHandle(HANDLE h);
	void SetProcessHandle(HANDLE h);

	// Settings.
	ResolutionType GetResolutionType() const;
	int GetTargetWidth() const;
	int GetTargetHeight() const;
	float GetTargetFramerate() const;
	bool ShouldRecordUI() const;
	std::string GetOutputPath() const;
	bool StopOnLastEvent() const;
	float GetStopDelayDuration() const;
	FFmpegEncoderConfig GetEncoderConfig() const;

	void SetResolutionType(ResolutionType type);
	void SetTargetFramerate(float framerate);
	void SetRecordUI(bool value);
	void SetStopOnLastEvent(bool value);
	void SetStopDelayDuration(float value);
	void SetOutputPath(std::string outputPath);
	void UpdateEncoderConfig(const FFmpegEncoderConfig newEncoderConfig);

	void Cleanup();

private:
	std::atomic<bool> m_StartRecording{ false };
	std::atomic<bool> m_StopRecording{ false };
	std::atomic<bool> m_IsRecording{ false };
	std::atomic<bool> m_IsCaptureActive{ false };
	std::atomic<float> m_RecordingSpeed{ 0.0f };
	std::atomic<TimePoint> m_StartRecordingTime{};
	
	std::atomic<HANDLE> m_hVideoPipe{ INVALID_HANDLE_VALUE };
	std::atomic<HANDLE> m_hAudioPipe{ INVALID_HANDLE_VALUE };
	std::atomic<HANDLE> m_hProcess{ INVALID_HANDLE_VALUE };

	std::atomic<ResolutionType> m_ResolutionType{ ResolutionType::UHD_4K };
	std::atomic<float> m_TargetFramerate{ 60.0f };
	std::atomic<bool> m_ShouldRecordUI{ false };
	std::atomic<bool> m_StopOnLastEvent{ false };
	std::atomic<float> m_StopDelayDuration{ 0.0f };
	
	std::string m_OutputPath{};
	mutable std::mutex m_Mutex;

	FFmpegEncoderConfig m_EncoderConfig;
	mutable std::mutex m_EncoderConfigMutex;
};