#pragma once

#include <vector>

// Video frame buffer with associated engine timestamp.
struct FrameData
{
	std::vector<uint8_t> buffer{};
	float engineTime = 0.0f;
};