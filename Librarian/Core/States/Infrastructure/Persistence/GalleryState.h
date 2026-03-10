#pragma once

#include "Core/Common/Types/GalleryTypes.h"
#include <vector>
#include <atomic>
#include <string>
#include <mutex>

struct GalleryState
{
public:
	std::vector<VideoData> GetVideos() const;
	void SetVideos(std::vector<VideoData> videos);

	VideoData GetVideo(int videoIndex) const;
	void AddVideo(VideoData videoData);
	void DeleteVideo(int videoIndex);

	void UpdateVideoName(int videoIndex, const std::string& newName);
	void UpdateVideoPath(int videoIndex, const std::string& newPath);
	void UpdateVideoMetadata(int videoIndex, float duration, void* thumb, const std::string& verifyPath);

	bool IsScanning(int videoIndex) const;
	void SetScanning(bool value);

	bool IsLoading(int videoIndex) const;
	void SetLoading(int videoIndex, bool value);

	int GetSelectedIndex() const;
	void SetSelectedIndex(int index);

	void Cleanup();

private:
	std::vector<VideoData> m_Videos;
	std::string m_CurrentPath;
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_IsScanning{ false };
	std::atomic<int> m_SelectedIndex{ -1 };
};