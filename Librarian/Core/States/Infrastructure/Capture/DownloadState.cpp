#include "pch.h"
#include "Core/States/Infrastructure/Capture/DownloadState.h"

bool DownloadState::IsFFmpegInstalled() const { return m_IsFFmpegInstalled.load(); }
bool DownloadState::IsDownloadInProgress() const { return m_IsDownloadInProgress.load(); }
float DownloadState::GetDownloadProgress() const { return m_DownloadProgress.load(); }

std::string DownloadState::GetExecutablePath() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_ExecutablePath;
}

void DownloadState::SetFFmpegInstalled(bool value) { m_IsFFmpegInstalled.store(value); }
void DownloadState::SetDownloadInProgress(bool value) { m_IsDownloadInProgress.store(value); }
void DownloadState::SetDownloadProgress(float value) { m_DownloadProgress.store(value); }

void DownloadState::SetExecutablePath(std::string path)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_ExecutablePath = path;
}