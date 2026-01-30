#pragma once

/** * @brief Maps specific game actions to their internal engine Button IDs.
 * These values correspond to the IDs expected by the engine's input polling logic.
 */
enum class InputAction
{
	Unknown = -1,

	// UI and Menu Controls.
	PauseMenu = 0,
	ToggleScoreboard = 30,

	// Theater Mode Controls.
	TogglePanel = 59,
	ToggleInterface = 58,
	TogglePOV = 60,
	CameraReset,			// Internal logic (No direct GetButtonState mapping).
	NextPlayer = 78,
	PreviousPlayer = 77,
	JumpForward = 80,
	JumpBackward = 79,
	PlayPause = 56,
	FastForward = 33,
	ToggleFreecam = 72,
	Boost = 57,
	FasterBoost = 69,
	Ascend = 34,
	Descend = 48,
	TheaterPanning,			// I didn't get this one.
};

/** * @brief Defines the operational scope for inputs.
 * Ensures the Director only injects commands when the engine is in the correct state.
 */
enum class InputContext
{
	Unknown, Communication, Movement,
	Actions, VehicleControls, UIControls,
	Theater, Forge,
};

/** * @brief Represents a paired action and context for the injection queue.
 */
struct GameInput
{
	InputContext InputContext;
	InputAction InputAction;
};

/** * @brief Data structure used to request an input injection with specific parameters.
 */
struct InputRequest
{
	InputAction Action;
	InputContext Context;
};