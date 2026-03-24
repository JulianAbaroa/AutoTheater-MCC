#pragma once

#include <memory>

// Capture
class FFmpegState;
class AudioState;
class VideoState;
class DownloadState;

// Engine
class InputState;
class LifecycleState;
class RenderState;

// Persistence
class GalleryState;
class ReplayState;
class SettingsState;

// Main container for the application's infrastructure states.
struct CoreInfrastructureState
{
	CoreInfrastructureState();
	~CoreInfrastructureState();

	std::unique_ptr<FFmpegState> FFmpeg;
	std::unique_ptr<AudioState> Audio;
	std::unique_ptr<VideoState> Video;
	std::unique_ptr<InputState> Input;
	std::unique_ptr<LifecycleState> Lifecycle;
	std::unique_ptr<RenderState> Render;
	std::unique_ptr<GalleryState> Gallery;
	std::unique_ptr<ReplayState> Replay;
	std::unique_ptr<SettingsState> Settings;
	std::unique_ptr<DownloadState> Download;
};