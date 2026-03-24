#include "pch.h"
#include "Core/States/Infrastructure/Persistence/GalleryState.h"
#include <d3d11.h>

VideoData GalleryState::GetVideo(int videoIndex) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (videoIndex >= 0 && videoIndex < (int)m_Videos.size())
	{
		return m_Videos[videoIndex];
	}

	return VideoData();
}

std::vector<VideoData> GalleryState::GetVideos() const 
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Videos;
}

void GalleryState::SetVideos(std::vector<VideoData> videos)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Videos = std::move(videos);
}

void GalleryState::AddVideo(VideoData videoData)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Videos.push_back(std::move(videoData));
}

void GalleryState::DeleteVideo(int videoIndex)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (videoIndex >= 0 && videoIndex < (int)m_Videos.size())
	{
		if (m_Videos[videoIndex].Thumbnail)
		{
			ID3D11ShaderResourceView* srv = (ID3D11ShaderResourceView*)m_Videos[videoIndex].Thumbnail;
			srv->Release();
			m_Videos[videoIndex].Thumbnail = nullptr;
		}

		m_Videos.erase(m_Videos.begin() + videoIndex);

		int currentSelected = m_SelectedIndex.load();
		if (currentSelected == videoIndex)
		{
			m_SelectedIndex.store(-1);
		}
		else if (currentSelected > videoIndex)
		{
			m_SelectedIndex.store(currentSelected - 1);
		}
	}
}


void GalleryState::UpdateVideoName(int videoIndex, const std::string& newName)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (videoIndex >= 0 && videoIndex < (int)m_Videos.size())
	{
		m_Videos[videoIndex].FileName = newName;

		std::filesystem::path oldPath = m_Videos[videoIndex].FullPath;
		std::filesystem::path newPath = oldPath.parent_path() / newName;

		m_Videos[videoIndex].FullPath = newPath.string();
	}
}


bool GalleryState::IsScanning(int videoIndex) const { return m_IsScanning.load(); }
int GalleryState::GetSelectedIndex() const { return m_SelectedIndex.load(); }
bool GalleryState::IsLoading(int videoIndex) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Videos[videoIndex].IsLoading;
}

void GalleryState::SetScanning(bool value) { m_IsScanning.store(value); }
void GalleryState::SetSelectedIndex(int index) { m_SelectedIndex.store(index); }

void GalleryState::SetLoading(int videoIndex, bool value)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Videos[videoIndex].IsLoading = value;
}


void GalleryState::UpdateVideoPath(int videoIndex, const std::string& newPath)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (videoIndex >= 0 && videoIndex < (int)m_Videos.size())
	{
		m_Videos[videoIndex].FullPath = newPath;
	}
}

void GalleryState::UpdateVideoMetadata(int videoIndex, float duration, void* thumb, const std::string& verifyPath)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (videoIndex >= 0 && videoIndex < (int)m_Videos.size())
	{
		m_Videos[videoIndex].IsLoading = false;

		if (m_Videos[videoIndex].FullPath.string() == verifyPath)
		{
			m_Videos[videoIndex].Duration = duration;
			m_Videos[videoIndex].Thumbnail = thumb;
			m_Videos[videoIndex].IsMetadataLoaded = true;
		}
	}
}


void GalleryState::Cleanup()
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	for (auto& video : m_Videos)
	{
		if (video.Thumbnail)
		{
			ID3D11ShaderResourceView* srv = (ID3D11ShaderResourceView*)video.Thumbnail;
			srv->Release();
			video.Thumbnail = nullptr;
		}
	}

	m_Videos = {};
}