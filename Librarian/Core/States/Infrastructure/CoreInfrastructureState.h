#pragma once

#include <memory>

struct FFmpegState;
struct AudioState;
struct VideoState;
struct InputState;
struct LifecycleState;
struct RenderState;
struct GalleryState;
struct ReplayState;
struct SettingsState;

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
};