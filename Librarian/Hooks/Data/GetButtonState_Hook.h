#pragma once

enum class InputAction
{
	Unknown,

	// UI Controls
	PauseMenu = 0,
	ToggleScoreboard = 30,

	// Theater
	TogglePanel = 59,		
	ToggleInterface = 58,
	TogglePOV = 60,
	CameraReset,			// It doesn't use the GetButtonState function
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
	TheaterPanning,			// I don't have this one
};

enum class InputContext
{
	Unknown, Communication, Movement,
	Actions, VehicleControls, UIControls,
	Theater, Forge,
};

struct GameInput
{
	InputContext InputContext;
	InputAction InputAction;
};

extern GameInput g_NextInput;

typedef char(__fastcall* GetButtonState_t)(short buttonID);

namespace GetButtonState_Hook
{
	void Install();
	void Uninstall();
}