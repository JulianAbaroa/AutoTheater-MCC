#include "pch.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Capture/ProcessState.h"
#include "Core/States/Infrastructure/Capture/PipeState.h"
#include "Core/States/Infrastructure/Capture/VideoState.h"
#include "Core/States/Infrastructure/Engine/InputState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/States/Infrastructure/Persistence/GalleryState.h"
#include "Core/States/Infrastructure/Persistence/ReplayState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/States/Infrastructure/Capture/DownloadState.h"

CoreInfrastructureState::CoreInfrastructureState()
{
	FFmpeg = std::make_unique<FFmpegState>();
	Process = std::make_unique<ProcessState>();
	Pipe = std::make_unique<PipeState>();
	Audio = std::make_unique<AudioState>();
	Video = std::make_unique<VideoState>();
	Input = std::make_unique<InputState>();
	Lifecycle = std::make_unique<LifecycleState>();
	Render = std::make_unique<RenderState>();
	Gallery = std::make_unique<GalleryState>();
	Replay = std::make_unique<ReplayState>();
	Settings = std::make_unique<SettingsState>();
	Download = std::make_unique<DownloadState>();
}

CoreInfrastructureState::~CoreInfrastructureState() = default;