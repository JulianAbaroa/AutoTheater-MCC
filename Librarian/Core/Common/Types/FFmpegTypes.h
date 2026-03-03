#pragma once

// Resolution dimensions in pixels.
struct ResolutionValues
{
	int Width;
	int Height;
};

// Supported resolution standards.
enum class ResolutionType
{
	FullHD,
	QHD,
	UHD_4K
};

// Map ResolutionType to its corresponding pixel dimensions.
struct TargetResolution
{
	static constexpr ResolutionValues Get(ResolutionType res)
	{
		switch (res)
		{
		case ResolutionType::FullHD: return { 1920, 1080 };
		case ResolutionType::QHD:    return { 2560, 1440 };
		case ResolutionType::UHD_4K: return { 3840, 2160 };
		default:                     return { 1920, 1080 };
		}
	}
};