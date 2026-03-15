#pragma once

#include <vector>

// Video frame buffer with associated engine timestamp.
struct FrameData
{
	std::vector<uint8_t> Buffer{};
	double RealTime = 0.0;
};