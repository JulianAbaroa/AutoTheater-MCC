#include "pch.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"

bool FFmpegState::RecordingStarted() const { return m_StartRecording.load(); }
bool FFmpegState::RecordingStopped() const { return m_StopRecording.load(); }
bool FFmpegState::IsRecording() const { return m_IsRecording.load(); }
bool FFmpegState::IsCaptureActive() const { return m_IsCaptureActive.load(); }
float FFmpegState::GetRecordingSpeed() const { return m_RecordingSpeed.load(); }
FFmpegState::TimePoint FFmpegState::GetStartRecordingTime() const { return m_StartRecordingTime.load(); }

void FFmpegState::SetStartRecording(bool value) { m_StartRecording.store(value); }
void FFmpegState::SetStopRecording(bool value) { m_StopRecording.store(value); }
void FFmpegState::SetRecording(bool value) { m_IsRecording.store(value); }
void FFmpegState::SetCaptureActive(bool value) { m_IsCaptureActive.store(value); }
void FFmpegState::SetRecordingSpeed(float value) { m_RecordingSpeed.store(value); }
void FFmpegState::SetStartRecordingTime(FFmpegState::TimePoint value) { m_StartRecordingTime.store(value); }


HANDLE FFmpegState::GetVideoPipeHandle() const { return m_hVideoPipe.load(); }
HANDLE FFmpegState::GetAudioPipeHandle() const { return m_hAudioPipe.load(); }
HANDLE FFmpegState::GetProcessHandle() const { return m_hProcess.load(); }

void FFmpegState::SetVideoPipeHandle(HANDLE h) { m_hVideoPipe.store(h); }
void FFmpegState::SetAudioPipeHandle(HANDLE h) { m_hAudioPipe.store(h); }
void FFmpegState::SetProcessHandle(HANDLE h) { m_hProcess.store(h); }


ResolutionType FFmpegState::GetResolutionType() const { return m_ResolutionType.load(); }
int FFmpegState::GetTargetWidth() const { return TargetResolution::Get(m_ResolutionType.load()).Width; }
int FFmpegState::GetTargetHeight() const { return TargetResolution::Get(m_ResolutionType.load()).Height; }
float FFmpegState::GetTargetFramerate() const { return m_TargetFramerate.load(); }
bool FFmpegState::ShouldRecordUI() const { return m_ShouldRecordUI.load(); }
bool FFmpegState::StopOnLastEvent() const { return m_StopOnLastEvent.load(); }
float FFmpegState::GetStopDelayDuration() const { return m_StopDelayDuration.load(); }

void FFmpegState::SetResolutionType(ResolutionType type) { m_ResolutionType.store(type); }
void FFmpegState::SetTargetFramerate(float framerate) { m_TargetFramerate.store(framerate); }
void FFmpegState::SetRecordUI(bool value) { m_ShouldRecordUI.store(value); }
void FFmpegState::SetStopOnLastEvent(bool value) { m_StopOnLastEvent.store(value); }
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

FFmpegEncoderConfig FFmpegState::GetEncoderConfig() const
{
	std::lock_guard<std::mutex> lock(m_EncoderConfigMutex);
	return m_EncoderConfig;
}

void FFmpegState::UpdateEncoderConfig(const FFmpegEncoderConfig newEncoderConfig)
{
	std::lock_guard<std::mutex> lock(m_EncoderConfigMutex);
	m_EncoderConfig = newEncoderConfig;
}

void FFmpegState::Cleanup()
{
	m_StartRecording.store(false);
	m_StopRecording.store(false);
	m_IsRecording.store(false);
	m_IsCaptureActive.store(false);
	m_RecordingSpeed.store(0.0f);
	m_StartRecordingTime.store({});

	m_hVideoPipe.store(INVALID_HANDLE_VALUE);
	m_hAudioPipe.store(INVALID_HANDLE_VALUE);
	m_hProcess.store(INVALID_HANDLE_VALUE);
}