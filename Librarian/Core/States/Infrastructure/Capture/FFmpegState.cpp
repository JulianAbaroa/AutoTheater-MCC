#include "pch.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"

bool FFmpegState::IsRecording() const { return m_IsRecording.load(); }
void FFmpegState::SetRecording(bool value) { m_IsRecording.store(value); }

bool FFmpegState::RecordingStarted() const { return m_StartRecording.load(); }
void FFmpegState::SetStartRecording(bool value) { m_StartRecording.store(value); }

bool FFmpegState::RecordingStopped() const { return m_StopRecording.load(); }
void FFmpegState::SetStopRecording(bool value) { m_StopRecording.store(value); }

bool FFmpegState::IsCaptureActive() const { return m_IsCaptureActive.load(); }
void FFmpegState::SetCaptureActive(bool value) { m_IsCaptureActive.store(value); }

FFmpegState::TimePoint FFmpegState::GetStartRecordingTime() const { return m_StartRecordingTime.load(); }
void FFmpegState::SetStartRecordingTime(FFmpegState::TimePoint value) { m_StartRecordingTime.store(value); }

int FFmpegState::GetTargetWidth() const { return TargetResolution::Get(m_ResolutionType.load()).Width; }
int FFmpegState::GetTargetHeight() const { return TargetResolution::Get(m_ResolutionType.load()).Height; }

float FFmpegState::GetTargetFramerate() const { return m_TargetFramerate.load(); }
void FFmpegState::SetTargetFramerate(float framerate) { m_TargetFramerate.store(framerate); }

ResolutionType FFmpegState::GetResolutionType() const { return m_ResolutionType.load(); }
void FFmpegState::SetResolutionType(ResolutionType type) { m_ResolutionType.store(type); }

bool FFmpegState::ShouldRecordUI() const { return m_ShouldRecordUI.load(); }
void FFmpegState::SetRecordUI(bool value) { m_ShouldRecordUI.store(value); }

bool FFmpegState::StopOnLastEvent() const { return m_StopOnLastEvent.load(); }
void FFmpegState::SetStopOnLastEvent(bool value) { m_StopOnLastEvent.store(value); }

float FFmpegState::GetStopDelayDuration() const { return m_StopDelayDuration.load(); }
void FFmpegState::SetStopDelayDuration(float value) { m_StopDelayDuration.store(value); }

std::string FFmpegState::GetOutputPath() const 
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_OutputPath;
}

void FFmpegState::SetOutputPath(std::string outputPath)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_OutputPath = outputPath;
}

FFmpegConfiguration FFmpegState::GetConfiguration() const
{
	std::lock_guard<std::mutex> lock(m_ConfigurationMutex);
	return m_Configuration;
}

void FFmpegState::UpdateConfiguration(const FFmpegConfiguration newEncoderConfig)
{
	std::lock_guard<std::mutex> lock(m_ConfigurationMutex);
	m_Configuration = newEncoderConfig;
}

void FFmpegState::Cleanup()
{
	m_IsRecording.store(false);
	m_StartRecording.store(false);
	m_StopRecording.store(false);
	m_IsCaptureActive.store(false);
	m_StartRecordingTime.store({});
}