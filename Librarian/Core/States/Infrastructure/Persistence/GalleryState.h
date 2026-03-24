#pragma once

#include "Core/Common/Types/GalleryTypes.h"
#include <vector>
#include <atomic>
#include <string>
#include <mutex>

class GalleryState
{
public:
	VideoData GetVideo(int videoIndex) const;
	std::vector<VideoData> GetVideos() const;
	void SetVideos(std::vector<VideoData> videos);
	void AddVideo(VideoData videoData);
	void DeleteVideo(int videoIndex);

	bool IsScanning(int videoIndex) const;
	int GetSelectedIndex() const;
	bool IsLoading(int videoIndex) const;

	void SetScanning(bool value);
	void SetSelectedIndex(int index);
	void SetLoading(int videoIndex, bool value);

	void UpdateVideoName(int videoIndex, const std::string& newName);
	void UpdateVideoPath(int videoIndex, const std::string& newPath);
	void UpdateVideoMetadata(int videoIndex, float duration, void* thumb, const std::string& verifyPath);

	void Cleanup();

private:
	std::vector<VideoData> m_Videos;
	std::string m_CurrentPath;
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_IsScanning{ false };
	std::atomic<int> m_SelectedIndex{ -1 };
};