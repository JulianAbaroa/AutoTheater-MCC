#pragma once

#include "Core/Common/Types/FFmpegTypes.h"
#include "Windows.h"
#include <string>

class PipeSystem
{
public:
	bool CreateVideoPipe(int width, int height);
	bool CreateAudioPipe();

	WriteResult TryWriteVideo(void* data, size_t size);
	bool WriteAudio(const void* data, size_t size);

	void Cleanup();
	
private:
	std::chrono::steady_clock::time_point m_LastVideoWriteTime{};
	std::chrono::steady_clock::time_point m_LastAudioWriteTime{};
};