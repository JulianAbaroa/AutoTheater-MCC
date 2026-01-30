#pragma once

/** * @brief Represents the operational phases of the AutoTheater engine.
 */
enum class AutoTheaterPhase
{
	// Standard operation. Only manual playback speed modifications are active.
	Default,	

	// Timeline Phase: The mod monitors and captures GameEvents to 
	// construct the replay's data structure.
	Timeline,	

	// Director Phase: The Director interprets the captured timeline 
	// to automate player POVs switches and playback adjustements.
	Director,		
};

/** * @brief Represents the current lifecycle state of the Blam! (Halo Reach) game engine.
 */ 
enum class EngineStatus
{
	// The engine is dormant in memory. It has either not been initialized 
	// yet or has been fuly deallocated.
	Awaiting,		

	// The engine is active and currently executing game logic.
	Running,

	// The engine instance has been torn down or destroyed.
	Destroyed,
};







