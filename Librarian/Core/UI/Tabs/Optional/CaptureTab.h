#pragma once

#include <mutex>
#include <chrono>

class CaptureTab
{
public:
	void Draw();
	
private:
	void DrawTopBar(bool isRecording);
	void DrawRecordingControls(bool recording, bool isCaptureActive);
	void DrawFFmpegControls(bool isRecoring, float totalWidth);

	void DrawGallery(bool isRecording);

	void DrawRecordingSettingsPopup(bool isRecording);
	void DrawRecordingSettings(bool isRecording);

	void DrawPopups();
	void DrawUninstallPopup();
	void DrawDeleteVideoPopup();
	void DrawTelemetryPopup();

	std::atomic<bool> m_IsReadyTocapture{ false };
	std::atomic<int> m_VideoIndexToDelete{ -1 };
	std::atomic<int> m_EditingVideoIndex{ -1 };
	char RenameVideoBuf[256] = "";

	std::atomic<bool> m_OpenRecordingSettingsModal{ false };
	std::atomic<bool> m_OpenUninstallModal{ false };
	std::atomic<bool> m_OpenDeleteVideoModal{ false };

	std::atomic<bool> m_FolderPickerActive{ false };
	std::string m_PendingNewPath = "";
	std::mutex m_Mutex;

	std::chrono::steady_clock::time_point m_StopRequestedTime{};
};