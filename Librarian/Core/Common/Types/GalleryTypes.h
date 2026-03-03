#pragma once

#include <filesystem>
#include <string>

// Video metadata and UI state for gallery display.
struct VideoData
{
	std::filesystem::path FullPath{};
	std::string FileName{};
	uint64_t FileSize = 0;
	float Duration = 0.0f;
	void* Thumbnail = nullptr;
	bool IsMetadataLoaded = false;
	bool IsLoading = false;
};