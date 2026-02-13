#pragma once

// States
#include "Core/States/Domain/TimelineState.h"
#include "Core/States/Domain/TheaterState.h"
#include "Core/States/Domain/DirectorState.h"
#include "Core/States/Domain/EventRegistryState.h"
#include "Core/States/Interface/InputState.h"
#include "Core/States/Interface/DebugState.h"
#include "Core/States/Infrastructure/ReplayState.h"
#include "Core/States/Infrastructure/SettingsState.h"
#include "Core/States/Infrastructure/RenderState.h"
#include "Core/States/Infrastructure/LifecycleState.h"

// Systems
#include "Core/Systems/Domain/TimelineSystem.h"
#include "Core/Systems/Domain/TheaterSystem.h"
#include "Core/Systems/Domain/DirectorSystem.h"
#include "Core/Systems/Domain/EventRegistrySystem.h"
#include "Core/Systems/Interface/InputSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Systems/Infrastructure/SettingsSystem.h"
#include "Core/Systems/Infrastructure/RenderSystem.h"
#include "Core/Systems/Infrastructure/LifecycleSystem.h"
#include "Core/Systems/Infrastructure/ReplaySystem.h"

#include <memory>

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
};

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
};

class AppCore
{
public:
	AppCore();
	~AppCore();

	std::unique_ptr<CoreSystem> System;
	std::unique_ptr<CoreState> State;
};

extern std::unique_ptr<AppCore> g_App;

extern CoreSystem* g_pSystem;
extern CoreState* g_pState;