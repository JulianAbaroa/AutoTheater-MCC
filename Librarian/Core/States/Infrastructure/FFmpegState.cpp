#include "pch.h"
#include "Core/States/Infrastructure/FFmpegState.h"

bool FFmpegState::RecordingStarted() const { return m_StartRecording.load(); }
void FFmpegState::SetStartRecording(bool value) { m_StartRecording.store(value); }

bool FFmpegState::RecordingStopped() const { return m_StopRecording.load(); }
void FFmpegState::SetStopRecording(bool value) { m_StopRecording.store(value); }

bool FFmpegState::IsRecording() const { return m_IsRecording.load(); }
void FFmpegState::SetRecording(bool value) { m_IsRecording.store(value); }

bool FFmpegState::IsCaptureActive() const { return m_IsCaptureActive.load(); }
void FFmpegState::SetCaptureActive(bool value) { m_IsCaptureActive.store(value); }

bool FFmpegState::StopOnLastEvent() const { return m_StopOnLastEvent.load(); }
void FFmpegState::SetStopOnLastEvent(bool value) { m_StopOnLastEvent.store(value); }

float FFmpegState::GetStopDelayDuration() const { return m_StopDelayDuration.load(); }
void FFmpegState::SetStopDelayDuration(float value) { m_StopDelayDuration.store(value); }

std::chrono::steady_clock::time_point FFmpegState::GetStartRecordingTime() const { return m_StartRecordingTime; }
void FFmpegState::SetStartRecordingTime(std::chrono::steady_clock::time_point time_point) { m_StartRecordingTime = time_point; }

HANDLE FFmpegState::GetVideoPipeHandle() const { return m_hVideoPipe.load(); }
void FFmpegState::SetVideoPipeHandle(HANDLE h) { m_hVideoPipe.store(h); }

HANDLE FFmpegState::GetAudioPipeHandle() const { return m_hAudioPipe.load(); }
void FFmpegState::SetAudioPipeHandle(HANDLE h) { m_hAudioPipe.store(h); }

HANDLE FFmpegState::GetProcessHandle() const { return m_hProcess.load(); }
void FFmpegState::SetProcessHandle(HANDLE h) { m_hProcess.store(h); }

ResolutionType FFmpegState::GetResolutionType() { return m_ResolutionType.load(); }
void FFmpegState::SetResolutionType(ResolutionType type) { m_ResolutionType.store(type); }

int FFmpegState::GetTargetWidth() { return TargetResolution::Get(m_ResolutionType.load()).Width; }
int FFmpegState::GetTargetHeight() { return TargetResolution::Get(m_ResolutionType.load()).Height; }

float FFmpegState::GetTargetFramerate() const { return m_TargetFramerate.load(); }
void FFmpegState::SetTargetFramerate(float framerate) { m_TargetFramerate.store(framerate); }

bool FFmpegState::ShouldRecordUI() const { return m_ShouldRecordUI.load(); }
void FFmpegState::SetRecordUI(bool value) { m_ShouldRecordUI.store(value); }

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

bool FFmpegState::IsFFmpegInstalled() const { return m_IsFFmpegInstalled.load(); }
void FFmpegState::SetFFmpegInstalled(bool value) { m_IsFFmpegInstalled.store(value); }

bool FFmpegState::IsDownloadInProgress() const { return m_IsDownloadInProgress.load(); }
void FFmpegState::SetDownloadInProgress(bool value) { m_IsDownloadInProgress.store(value); }

float FFmpegState::GetDownloadProgress() const { return m_DownloadProgress.load(); }
void FFmpegState::SetDownloadProgress(float value) { m_DownloadProgress.store(value); }


std::string FFmpegState::GetExecutablePath() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_ExecutablePath;
}

void FFmpegState::SetExecutablePath(std::string path)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_ExecutablePath = path;
}


void FFmpegState::Cleanup()
{
	m_StartRecording.store(false);
	m_StopRecording.store(false);
	m_IsRecording.store(false);

	m_hVideoPipe.store(INVALID_HANDLE_VALUE);
	m_hAudioPipe.store(INVALID_HANDLE_VALUE);
	m_hProcess.store(INVALID_HANDLE_VALUE);
}