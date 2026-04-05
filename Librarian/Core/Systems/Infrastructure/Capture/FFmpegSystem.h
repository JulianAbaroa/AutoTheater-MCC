#pragma once

#include "Core/Common/Types/FFmpegTypes.h"
#include <string>
#include <atomic>

class FFmpegSystem
{
public:
	bool Start(const std::string& outputPath, int width, int height, float framerate);
	void ForceStop();
	void Stop();

	float GetRecordingDuration() const;
	void Cleanup();

private:
	void InternalStop(bool force);
};