#pragma once

#include "Core/States/Domain/TimelineState.h"
#include "Core/States/Domain/TheaterState.h"
#include "Core/States/Domain/DirectorState.h"
#include "Core/States/Domain/EventRegistryState.h"
#include "Core/States/Interface/InputState.h"
#include "Core/States/Interface/DebugState.h"
#include "Core/States/Infrastructure/ReplayState.h"
#include "Core/States/Infrastructure/SettingsState.h"
#include "Core/States/Infrastructure/RenderState.h"
#include "Core/States/Infrastructure/AudioState.h"
#include "Core/States/Infrastructure/VideoState.h"
#include "Core/States/Infrastructure/FFmpegState.h"
#include "Core/States/Infrastructure/LifecycleState.h"
#include "Core/States/Infrastructure/GalleryState.h"

struct CoreState
{
	// Domain
	TimelineState Timeline;
	TheaterState Theater;
	DirectorState Director;
	EventRegistryState EventRegistry;

	// Interface
	InputState Input;
	DebugState Debug;

	// Infrastructure
	SettingsState Settings;
	RenderState Render;
	LifecycleState Lifecycle;
	ReplayState Replay;
	AudioState Audio;
	VideoState Video;
	FFmpegState FFmpeg;
	GalleryState Gallery;
};

extern CoreState* g_pState;