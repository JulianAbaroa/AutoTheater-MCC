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

// Base data structure for Game Events used by BuildGameEvent.
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

struct ReplayModule
{
	void* VTable1;						// 0x00
	void* VTable2;						// 0x08

	uint32_t hFollowedBiped;			// 0x10
	std::byte _pad1[12];

	uint32_t hFirstPerson;				// 0x20
	std::byte _pad2[12];

	std::byte _pad3[16];
	std::byte _pad4[16];
	std::byte _pad5[16];

	std::byte _pad6[8];
	uint32_t hFollowedBiped2;			// 0x68
	std::byte _pad7[4];

	std::byte _pad8[16];
	std::byte _pad9[16];
	std::byte _pad10[16];

	uint32_t hFirstPerson2;				// 0xA0
	float CameraForward[3];				// 0xA4

	float CameraUp[3];					// 0xB0
	std::byte _pad11[4];

	std::byte _pad12[16];
	std::byte _pad13[16];
	std::byte _pad14[16];
	std::byte _pad15[16];

	float FollowedBipedPos[3];			// 0x100
	std::byte _pad16[4];

	uint32_t hFollowedBiped3;			// 0x110
	std::byte _pad17[4];
	std::byte IsTrackingValid;			// 0x118
	std::byte _pad18[3 + 4];

	float BipedPosY;					// 0x124
	float BipedPosX;					// 0x128
	std::byte _pad19[4];
	std::byte _pad20[4];

	std::byte _pad21[16];
	std::byte _pad22[16];
	std::byte _pad23[16];
	std::byte _pad24[16];
	std::byte _pad25[16];

	std::byte _pad26[4];
	std::byte FollowedPlayerIndex;		// 0x184
	std::byte _pad27;
	uint16_t FollowedPlayerSlotID;		// 0x186
	uint32_t hUnknown;					// 0x18A
	std::byte _pad28[4];

	std::byte POVMode;					// 0x192
	std::byte _pad29[3];
	std::byte UIMode;					// 0x196
	std::byte _pad30[3];
	std::byte CameraMode;				// 0x19A
	std::byte _pad31[3 + 4];

	std::byte _pad32[16];
	std::byte _pad33[16];
	std::byte _pad34[16];
	std::byte _pad35[16];
	std::byte _pad36[16];
};

#pragma pack(pop)