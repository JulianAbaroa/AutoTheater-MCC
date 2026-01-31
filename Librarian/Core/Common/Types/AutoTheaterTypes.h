#pragma once

// Represents the operational phases of the AutoTheater engine.
enum class AutoTheaterPhase
{
	// Only manual playback speed modifications are active.
	Default,	

	// AutoTheater monitors and captures GameEvents to 
	// construct the replay's data structure.
	Timeline,	

	// Director interprets the captured timeline to automate 
	// player POVs switches and playback adjustements.
	Director,		
};

// Represents the current lifecycle state of the Blam! (Halo Reach) game engine.
enum class EngineStatus
{
	// The engine it has either not been initialized yet or has been fuly deallocated.
	Awaiting,		

	// The engine is active and currently executing game logic.
	Running,

	// The engine instance has been torn down or destroyed.
	Destroyed,
};







