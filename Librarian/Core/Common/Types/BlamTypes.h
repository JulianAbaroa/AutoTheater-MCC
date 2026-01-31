#pragma once

#include <cstdint>
#include <cstddef>

#pragma pack(push, 1)

// Player structure within the global PlayerTable.
struct RawPlayer
{
	// Unique Identifier, likely networking-related.
	uint32_t SlotID;
	std::byte _pad1[36];

	// Biped Handle: Active while the player is alive.
	uint32_t hCurrentBiped;

	// Biped Handle: Persists after death if a biped was previously controlled.
	uint32_t hPreviousBiped;
	std::byte _pad2[4];

	// Biped Handle: Primary reference as long as the biped exists and belongs to this player.
	uint32_t hBiped;

	// Spatial Data: World-space coordinates and orientation.
	float Position[3];
	float Rotation[3];
	float LookVector[3];

	// Equipment Handles: Stored while item is primary/secondary and owned by this player.
	uint32_t hPrimaryWeapon;
	uint32_t hSecondaryWeapon;

	// Objective Handle: Stores active objective item (Flag, Bomb, etc.) while held.
	uint32_t hObjective;
	std::byte _pad3[72];

	// Player Identity: Username and Service Tag.
	wchar_t Name[28];
	std::byte _pad4[12];
	wchar_t Tag[8];
	std::byte _pad5[908];
};

// Physical biped object in the game world.
struct RawBiped
{
	// Dynamic Class ID: Session-specific indentifier for the biped.
	uint32_t ClassID;
	std::byte _pad1[28];

	// Spatial Data: Current world position.
	float Position[3];
	std::byte _pad2[66];

	// ... additional fields.
};

// Weapon object data. 
// Note: Structure may vary across different weapon classes.
struct RawWeapon
{
	// Dynamic Class ID: Session-specific identifier for this weapon type.
	uint32_t ClassID;
	std::byte _pad1[8];

	uint32_t UnknownHandle2;
	std::byte _pad2[4];

	// Owner Handle: The biped currently holding this weapon.
	uint32_t hBiped;
	std::byte _pad3[8];

	// Spatial Data: World position of the weapon object.
	float Position[3];
	std::byte _pad4[400];

	// Internal object handle for this specific weapon instance (may be wrong).
	uint32_t WeaponHandle;
	std::byte _pad5[266];

	// State Data: Current ammunition conunt (Verified for DMR).
	short CurrentAmmo;
	std::byte _pad6[484];

	// ... additional fields.
};

// Base data structure for Game Events used by UIBuildDynamicMessage.
// Defines interaction between a source (Cause) and a target (Effect).
struct EventData
{
	// Cause data (The instigator of the event).
	uint8_t CauseSlotIndex;		// Index in PlayerTable.
	uint8_t _pad1;
	uint16_t CauseSalt;			// Corresponds to Player SlotID.
	uint32_t CauseHandle;		// Corresponds to Player hBiped.

	// Effect data (The target affected by the event).
	uint8_t EffectSlotIndex;	// Index in PlayerTable.
	uint8_t _pad2;
	uint16_t EffectSalt;		// Corresponds to Player SlotID.
	uint32_t EffectHandle;		// Corresponds to Player hBiped.

	// Team Metadata (Valid range: 0-7).
	int8_t CauseTeam;
	int8_t EffectTeam;

	std::byte _pad3[10];

	// Event-specific data field.
	uint16_t CustomValue;

	std::byte _pad4[2];
};

#pragma pack(pop)