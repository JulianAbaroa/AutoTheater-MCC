#include "pch.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/AudioSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Infrastructure/Capture/ProcessSystem.h"
#include "Core/Systems/Infrastructure/Capture/PipeSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"
#include "Core/Systems/Infrastructure/Engine/InputSystem.h"
#include "Core/Systems/Infrastructure/Engine/LifecycleSystem.h"
#include "Core/Systems/Infrastructure/Engine/RenderSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Infrastructure/Persistence/GallerySystem.h"
#include "Core/Systems/Infrastructure/Persistence/PreferencesSystem.h"
#include "Core/Systems/Infrastructure/Persistence/ReplaySystem.h"
#include "Core/Systems/Infrastructure/Persistence/SettingsSystem.h"
#include "Core/Systems/Infrastructure/Engine/ThreadSystem.h"
#include "Core/Systems/Infrastructure/Engine/FormatSystem.h"
#include "Core/Systems/Infrastructure/Engine/DialogSystem.h"
#include "Core/Systems/Infrastructure/Capture/DownloadSystem.h"
#include "Core/Systems/Infrastructure/Capture/SyncSystem.h"

CoreInfrastructureSystem::CoreInfrastructureSystem()
{
	Audio = std::make_unique<AudioSystem>();
	FFmpeg = std::make_unique<FFmpegSystem>();
	Process = std::make_unique<ProcessSystem>();
	Pipe = std::make_unique<PipeSystem>();
	Video = std::make_unique<VideoSystem>();
	Input = std::make_unique<InputSystem>();
	Lifecycle = std::make_unique<LifecycleSystem>();
	Render = std::make_unique<RenderSystem>();
	Scanner = std::make_unique<ScannerSystem>();
	Gallery = std::make_unique<GallerySystem>();
	Preferences = std::make_unique<PreferencesSystem>();
	Replay = std::make_unique<ReplaySystem>();
	Settings = std::make_unique<SettingsSystem>();
	Thread = std::make_unique<ThreadSystem>();
	Format = std::make_unique<FormatSystem>();
	Dialog = std::make_unique<DialogSystem>();
	Download = std::make_unique<DownloadSystem>();
	Sync = std::make_unique<SyncSystem>();
}

CoreInfrastructureSystem::~CoreInfrastructureSystem() = default;