#pragma once

struct Signature
{
	const char* name;
	const char* pattern;
};

namespace Signatures
{
	// Functions
	const Signature FilmPlayerStateDecode = {
		"FilmPlayerStateDecode",
		"48 8B C4 48 89 58 ?? 48 89 70 ?? 48 89 78 ?? 55 41 54 41 55 41 56 41 57 48 8D 68 ?? 48 81 EC 20 01 00 00 0F 29 70 ?? 0F 29 78 ?? 44 0F 29 40 ?? 44 0F 29 48 ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 D0",
	};

	const Signature SpectatorHandleInput = {
		"SpectatorHandleInput",
		"48 8B C4 48 89 58 10 48 89 68 18 48 89 70 20 57 48 83 EC 30 0F 29 70 E8 48 8B D9 8B 89 80 01 00 00",
	};

	const Signature UIBuildDynamicMessage = {
		"UIBuildDynamicMessage",
		"48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 44 89 48 20 57 41 54 41 55 41 56 41 57 48 83 EC 40 4C 8B B4 24 90 00 00 00",
	};

	const Signature UpdateTelemetryTimer = {
		"UpdateTelemetryTimer",
		"48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 57 48 83 EC 30 0F 57 D2 0F 29 70 E8 0F 28 C1",
	};

	const Signature DestroySubsystems = {
		"DestroySubsystems",
		"40 56 57 41 57 48 83 EC 40 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 68 48 89 6C 24 70 33 F6",
	};

	const Signature EngineInitialize = {
		"EngineInitialize",
		"48 8B C4 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC C0 00 00 00 48 C7 45 B7 FE FF FF FF",
	};

	const Signature BlamOpenFile = {
		"BlamOpenFile",
		"48 89 5C 24 10 55 56 57 48 81 EC 50 03 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 84 24 40 03 00 00",
	};

	const Signature FilmInitializeState = {
		"FilmInitializeState",
		"48 89 5C 24 18 55 56 57 48 8D AC 24 A0 26 FE FF B8 60 DA 01 00 E8",
	};

	const Signature GetButtonState = {
		"GetButtonState",
		"?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 0F BF D1 83 EA 69 74 ?? 83 EA 01 74 ?? 83 EA 01 74 ?? 83 FA 01 74"
	};

	// Variables
	const Signature TimeScaleModifier = {
		"TimeScaleModifier",
		"F3 0F 11 05 ?? ?? ?? ?? EB 0A C7 05",
	};

	const Signature TimeModifier = {
		"ReplayTimeModifier",
		"F3 0F 11 35 ?? ?? ?? ?? 0F 28 74 24 20 F3 0F 11 05 ?? ?? ?? ?? F3 0F 11 0D",
	};

	const Signature TelemetryIdModifier = {
		"TelemetryIdModifier",
		"8B 0D ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 48 8B 04 C8",
	};
}