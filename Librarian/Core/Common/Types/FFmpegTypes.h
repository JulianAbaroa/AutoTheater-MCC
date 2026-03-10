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

enum class VideoPreset
{
	P1, P2, P3, P4, P5, P6, P7
};

enum class ScalingFilter
{
	Bicubic, Lanczos, Bilinear, Spline
};

enum class OutputContainer 
{
	MKV, MP4
};

enum class EncoderType
{
	NVENC, AMF, QSV, CPU
};

struct FFmpegEncoderConfig
{
	int ThreadQueueSize = 128;
	int BitrateKbps = 80000;
	int VideoBufferPipeSize = 256;
	VideoPreset VideoPreset = VideoPreset::P1;
	ScalingFilter ScalingFilter = ScalingFilter::Bicubic;
	OutputContainer OutputContainer = OutputContainer::MKV;
	EncoderType EncoderType = EncoderType::CPU;
};