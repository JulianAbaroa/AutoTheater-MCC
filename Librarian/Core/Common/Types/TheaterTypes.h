#pragma once

#include <cstdint>

enum class POVMode : int8_t
{
	Unselected = -1,
	ThirdPersonAttached = 0,
	ThirdPersonFree = 1,
	Unknown = 2,
	FirstPerson = 3
};

enum class UIMode : int8_t
{
	Unselected = -1,
	Theater = 0,
	Normal = 1,
	None = 2
};

enum class CameraMode : int8_t
{
	Unselected = -1,
	Following = 0,
	Free = 1
};