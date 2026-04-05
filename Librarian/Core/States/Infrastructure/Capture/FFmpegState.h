#pragma once

#include "Core/Common/Types/FFmpegTypes.h"
#include <string>
#include <atomic>
#include <mutex>

class FFmpegState 
{
public:
	using TimePoint = std::chrono::steady_clock::time_point;
	
	// Recording-related.
	bool IsRecording() const;
	void SetRecording(bool value);

	bool RecordingStarted() const;
	void SetStartRecording(bool value);

	bool RecordingStopped() const;
	void SetStopRecording(bool value);

	bool IsCaptureActive() const;
	void SetCaptureActive(bool value);

	TimePoint GetStartRecordingTime() const;
	void SetStartRecordingTime(TimePoint value);

	// Configuration.
	int GetTargetWidth() const;
	int GetTargetHeight() const;

	ResolutionType GetResolutionType() const;
	void SetResolutionType(ResolutionType type);

	float GetTargetFramerate() const;
	void SetTargetFramerate(float framerate);

	bool ShouldRecordUI() const;
	void SetRecordUI(bool value);

	bool StopOnLastEvent() const;
	void SetStopOnLastEvent(bool value);

	float GetStopDelayDuration() const;
	void SetStopDelayDuration(float value);

	FFmpegConfiguration GetConfiguration() const;
	void UpdateConfiguration(const FFmpegConfiguration newConfiguration);

	std::string GetOutputPath() const;
	void SetOutputPath(std::string outputPath);

	void Cleanup();

private:
	// Recording-related.
	std::atomic<bool> m_IsRecording{ false };
	std::atomic<bool> m_StartRecording{ false };
	std::atomic<bool> m_StopRecording{ false };
	std::atomic<bool> m_IsCaptureActive{ false };
	std::atomic<TimePoint> m_StartRecordingTime{};

	// Configuration.
	std::atomic<float> m_TargetFramerate{ 0.0f };
	std::atomic<ResolutionType> m_ResolutionType{ ResolutionType::FullHD };
	std::atomic<bool> m_ShouldRecordUI{ false };
	std::atomic<bool> m_StopOnLastEvent{ false };
	std::atomic<float> m_StopDelayDuration{ 0.0f };
	
	std::string m_OutputPath{};
	mutable std::mutex m_Mutex;

	FFmpegConfiguration m_Configuration;
	mutable std::mutex m_ConfigurationMutex;
};