#include "pch.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/AudioSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"
#include "Core/Systems/Infrastructure/Engine/InputSystem.h"
#include "Core/Systems/Infrastructure/Engine/LifecycleSystem.h"
#include "Core/Systems/Infrastructure/Engine/RenderSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Infrastructure/Persistence/GallerySystem.h"
#include "Core/Systems/Infrastructure/Persistence/PreferencesSystem.h"
#include "Core/Systems/Infrastructure/Persistence/ReplaySystem.h"
#include "Core/Systems/Infrastructure/Persistence/SettingsSystem.h"

CoreInfrastructureSystem::CoreInfrastructureSystem()
{
	Audio = std::make_unique<AudioSystem>();
	FFmpeg = std::make_unique<FFmpegSystem>();
	Video = std::make_unique<VideoSystem>();
	Input = std::make_unique<InputSystem>();
	Lifecycle = std::make_unique<LifecycleSystem>();
	Render = std::make_unique<RenderSystem>();
	Scanner = std::make_unique<ScannerSystem>();
	Gallery = std::make_unique<GallerySystem>();
	Preferences = std::make_unique<PreferencesSystem>();
	Replay = std::make_unique<ReplaySystem>();
	Settings = std::make_unique<SettingsSystem>();
}

CoreInfrastructureSystem::~CoreInfrastructureSystem() = default;