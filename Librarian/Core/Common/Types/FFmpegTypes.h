#pragma once

#include <string>

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
	int BitrateKbps = 80000;
	int MaxBufferedFrames = 60;
	int MaxAudioBufferedPackets = 512;
	VideoPreset VideoPreset = VideoPreset::P1;
	ScalingFilter ScalingFilter = ScalingFilter::Bicubic;
	OutputContainer OutputContainer = OutputContainer::MKV;
	EncoderType EncoderType = EncoderType::CPU;
};

struct CaptureTelemetry
{
	// CaptureThread pendings.
	size_t VideoPendingQueueSize = 0;
	size_t AudioPendingQueueSize = 0;

	// Windows (kernel) pendings.
	DWORD VideoPipeBytesPending = 0;
	DWORD AudioPipeBytesPending = 0;
	DWORD VideoPipeBufferSize = 0;
	DWORD AudioPipeBufferSize = 0;

	// Latencies.
	float LastVideoWriteLatencyMs = 0.0f;
	float LastAudioWriteLatencyMs = 0.0f;
	float MaxVideoWriteLatencyMs = 0.0f;
	float MaxAudioWriteLatencyMs = 0.0f;

	// FFmpeg states.
	float FFmpegSpeed = 0.0f;
	float CurrentBitrateKbps = 0.0f;
};