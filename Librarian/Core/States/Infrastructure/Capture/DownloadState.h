#pragma once

#include <atomic>
#include <string>
#include <mutex>

class DownloadState
{
public:
	bool IsFFmpegInstalled() const;
	bool IsDownloadInProgress() const;
	float GetDownloadProgress() const;
	std::string GetExecutablePath() const;

	void SetFFmpegInstalled(bool value);
	void SetDownloadInProgress(bool value);
	void SetDownloadProgress(float value);
	void SetExecutablePath(std::string path);

private:
	std::atomic<bool> m_IsFFmpegInstalled{ false };
	std::atomic<bool> m_IsDownloadInProgress{ false };
	std::atomic<float> m_DownloadProgress{ 0.0f };

	std::string m_ExecutablePath;
	mutable std::mutex m_Mutex;
};