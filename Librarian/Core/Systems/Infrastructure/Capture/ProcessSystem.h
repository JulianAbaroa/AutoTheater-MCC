#pragma once

#include "Windows.h"
#include <string>
#include <atomic>
#include <mutex>

class ProcessSystem
{
public:
	void InitializeDependencies();
	bool VerifyExecutable(const std::string& path);

	std::string BuildFFmpegCommand(std::string outputPath, int width, int height, float fps);
	bool LaunchFFmpeg(const std::string& cmd);

	void ReadLogsThread(HANDLE hPipe);
	void TranslateAndLog(const std::string& logLine);

	void Cleanup();

private:
	std::string GenerateTimestampName();
};