#pragma once

#include "Core/Systems/Domain/TimelineSystem.h"
#include "Core/Systems/Domain/TheaterSystem.h"
#include "Core/Systems/Domain/DirectorSystem.h"
#include "Core/Systems/Domain/EventRegistrySystem.h"
#include "Core/Systems/Interface/InputSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Systems/Infrastructure/SettingsSystem.h"
#include "Core/Systems/Infrastructure/RenderSystem.h"
#include "Core/Systems/Infrastructure/AudioSystem.h"
#include "Core/Systems/Infrastructure/VideoSystem.h"
#include "Core/Systems/Infrastructure/FFmpegSystem.h"
#include "Core/Systems/Infrastructure/LifecycleSystem.h"
#include "Core/Systems/Infrastructure/ReplaySystem.h"
#include "Core/Systems/Infrastructure/GallerySystem.h"
#include "Core/Systems/Infrastructure/UserPreferencesSystem.h"
#include "Core/Systems/Infrastructure/ScannerSystem.h"

struct CoreSystem
{
	// Domain
	TimelineSystem Timeline;
	TheaterSystem Theater;
	DirectorSystem Director;
	EventRegistrySystem EventRegistry;

	// Interface
	InputSystem Input;
	DebugSystem Debug;

	// Infrastructure
	SettingsSystem Settings;
	RenderSystem Render;
	LifecycleSystem Lifecycle;
	ReplaySystem Replay;
	AudioSystem Audio;
	VideoSystem Video;
	FFmpegSystem FFmpeg;
	GallerySystem Gallery;
	UserPreferencesSystem Preferences;
	ScannerSystem Scanner;
};

extern CoreSystem* g_pSystem;