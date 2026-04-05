#pragma once

#include "Windows.h"
#include <string>
#include <chrono>

class PipeSystem
{
public:
	bool CreatePipes(std::string& videoPipeName, std::string& audioPipeName, int width, int height);
	bool WaitForVideoPipe() const;

	bool WriteVideo(void* data, size_t size);
	bool WriteAudio(const void* data, size_t size);
	bool WriteWithTimeout(HANDLE hPipe, const void* data, size_t size, DWORD timeoutMs, bool isVideo);

	void Cleanup();

private:
	std::chrono::steady_clock::time_point m_LastVideoWriteTime{};
	std::chrono::steady_clock::time_point m_LastAudioWriteTime{};
};