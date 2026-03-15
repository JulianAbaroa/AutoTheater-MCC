#include "pch.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Capture/VideoState.h"
#include "Core/States/Infrastructure/Engine/InputState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/States/Infrastructure/Persistence/GalleryState.h"
#include "Core/States/Infrastructure/Persistence/ReplayState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/States/Infrastructure/Capture/MuxerState.h"

CoreInfrastructureState::CoreInfrastructureState()
{
	FFmpeg = std::make_unique<FFmpegState>();
	Audio = std::make_unique<AudioState>();
	Video = std::make_unique<VideoState>();
	Input = std::make_unique<InputState>();
	Lifecycle = std::make_unique<LifecycleState>();
	Render = std::make_unique<RenderState>();
	Gallery = std::make_unique<GalleryState>();
	Replay = std::make_unique<ReplayState>();
	Settings = std::make_unique<SettingsState>();
	Muxer = std::make_unique<MuxerState>();
}

CoreInfrastructureState::~CoreInfrastructureState() = default;