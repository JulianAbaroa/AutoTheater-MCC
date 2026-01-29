#pragma once

/**
 * @file Types.h
 * @project AutoTheater (Halo Reach / Blam! Engine)
 * @target_module haloreach.dll (Version 1.3528.0.0)
 * @build_id Steam Build 19905945
 * * @description
 * Definitions of memory structures and game-state data types.
 * Memory alignment is critical; #pragma pack(1) is used to match
 * the Blam! engine's internal memory layout.
 */

#include <filesystem>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

#pragma pack(push, 1)

/** * @brief Player structure within the global PlayerTable.
 * Contains spatial, networking and state data for each player. 
 */
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

/** * @brief Physical biped representation in the game world.
 */
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

/** * @brief Weapon object data.
 * Note: Structure may vary across different weapon classes.
 */
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

/** * @brief Base data structure for Game Events used by UIBuildDynamicMessage.
 * Defines interaction between a source (Cause) and a target (Effect).
 */ 
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
	Idle,		

	// The engine is active and currently executing game logic.
	Running,

	// The engine instance has been torn down or destroyed.
	Destroyed,
};



/** * @brief Categories all identifiable game occurrences captured from the engine.
 * These events serve as data points for the Timeline. 
 */ 
enum class EventType
{
	// Fallback and internal logic.
	Unknown, Ignore,

	// Server-side and connection lifecycle.
	Join, Rejoin, Quit, Booted, JoinedTeam, 

	// Match progression and timing milestones.
	TookLead, LostLead, TiedLead, OneMinuteRemaining,
	OneMinuteToWin, ThirtySecondsToWin, TenSecondsRemaining,
	ThirtySecondsRemaining, RoundOver, GameOver, Wins,

	// Extensibility placeholder.
	Custom,

	// Capture The Flag (CTF) specific events.
	CaptureTheFlag, FlagStolen, FlagCaptured, FlagReseted,
	FlagRecovered, FlagDropped,

	// Assault specific events.
	Assault, HasTheBomb, DroppedTheBomb, ArmedTheBomb,
	TeamScored, BombReset,

	// Slayer specific events.
	Slayer,

	// Juggernaut specific events.
	Juggernaut, IsTheJuggernaut, YouKilledTheJuggernaut,

	// Race specific events.
	Race,

	// King Of The Hill (KOTH) specific events.
	KingOfTheHill, ControlsTheHill, HillContested, HillMoved,

	// Territories specific events.
	Territories, TeamCapturedATerritory, TerritoryContested,
	TerritoryLost,

	// Infection specific events.
	Infection, Infected, IsTheLastManStanding, ZombiesWin,
	SurvivorsWin, YouAreAZombie,

	// Oddball specific events.
	Oddball, PlayBall, BallReset, PickedUpTheBall, BallDropped,

	// Combat, Medals, and Kill-related events.
	Headshot, Pummeled, Stuck, KillingSpree, Splattered,
	BeatDown, Assassinated, KillingFrenzy, RunningRiot,
	Lasered, Sniped, Rampage, Untouchable, Invincible,
	Inconceivable, Unfrigginbelievable, Stopped, KillWithGrenades,
	Kill, Betrayal, Suicide, Struck, DoubleKill, TripleKill,
	SwordSpree, ReloadThis, Revenge, Killjoy, Avenger, Pull,
	Headcase, Assist, CloseCall, FirstStrike, Firebird, Protector,
	Overkill, HammerSpree, SplatterSpree, VehicularManslaughter,
	SundayDriver, SliceNDice, CuttingCrew, StickySpree, StickyFingers,
	Corrected, Dreamcrusher, WreckingCrew, ShotgunSpree, OpenSeason,
	BuckWild, Killtacular, Killtrocity, Killimanjaro, Killtastrophe,
	Killpocalypse, Killionaire, Bulltrue, KilledByTheGuardians, Yoink,
	KillFromTheGrave, JuggernautSpree, FellToYourDeath, Wheelman,
	WheelmanSpree, Roadhog, Roadrage, AssistSpree, Sidekick, SecondGunman,
	ShowStopper, SpawnSpree, Wingman, Broseidon,

	// Add new game events here...
};

/** * @brief Metadata associated with a specific game event.
 */ 
struct EventInfo
{
	// The categorized type of the event used for logic identification.
	EventType Type;

	// The priority score assigned to this event.
	// Higher weights increase the likelihood of the Director focusing on this event.
	int Weight;
};

/** * @brief Simplified structure to represent team interactions within an event.
 */ 
struct EventTeams
{
	// Maps to the CauseTeam (Instigator)
	int8_t First;

	// Maps to the EffectTeam (Target)
	int8_t Second;
};

/** * @brief High-level representation of a player, aggregating raw engine data
 * with processed metadata for UI and Director logic. 
 */ 
struct PlayerInfo {
	// Snapshotted spatial and state data directly from the game's PlayerTable.
	RawPlayer RawPlayer{};

	// Collection of weapon objects currently associated with or held by the player.
	std::vector<RawWeapon> Weapons{};

	// Processed UTF-8 string of the player's username.
	std::string Name;

	// Processed UTF-8 string of the player's service tag.
	std::string Tag;

	// Unique identifier representing the player's index within the PlayerTable.
	uint8_t Id = 0;
};

/** * @brief A processed event entry within the AutoTheater timeline.
 * Aggregates timing, types, and all participating player datafor logic evaluation.
 */
struct GameEvent
{
	// The specific time (in seconds) the event occurred within the replay.
	float Timestamp;

	// The categorized nature of the event (Kill, Capture, Join, etc.).
	EventType Type;

	// Simplified team context (Cause vs. Effect team IDs).
	EventTeams Teams;

	// List of the players involved in this specific event instance.
	std::vector<PlayerInfo> Players;
};



/** * @brief UI-specific metadata used for rendering on ImGui.
 */ 
struct EventMetadata
{
	// Human-readable summary of the event shown in the UI.
	std::string Description;

	// List of internal engine string templates associated with this event category.
	// Used to map raw engine strings to this specific metadata entry.
	std::vector<std::wstring> InternalTemplates;
};

struct SavedReplay
{
	std::string Hash;
	std::string DisplayName;
	std::string MovFileName;
	std::string Author;
	std::string Info;
	std::filesystem::path FullPath;
	bool HasTimeline;
};

struct CurrentFilmMetadata
{
	std::string Author;
	std::string Info;
};



/** * @brief Defines the types of automated actions the Director can perform.
 */ 
enum class CommandType 
{ 
	// Immediate camera switch to a specific player's perspective.
	Cut, 

	// Modification of the Theater playback speed.
	SetSpeed 
};

/** * @brief An instruction generated by the Director to be executed during playback.
 * Contains the 'what', 'when', and 'why' of an automated theater action.
 */ 
struct DirectorCommand
{
	// The time within the replay when the command should trigger.
	float Timestamp;

	// The nature of the action (e.g, switching cameras or changing speed).
	CommandType Type;

	// The index and name of the player the Director decided to focus on.
	uint8_t TargetPlayerIdx;
	std::string TargetPlayerName;

	// The playback speed value (used if Type is SetSpeed).
	float SpeedValue;

	// A summary of the Director's heuristic decision.
	// Format: "Score [X], Events[Y]", used for debug/UI.
	std::string Reason;
};

/** * @brief Represents a continuous window of high-interest gameplay.
 * Created during the script generation by clustering events that involve
 * the same player within a temporal look-ahead window.
 */ 
struct ActionSegment
{
	// Collection of EventTypes that occurred within this segment's duration.
	std::vector<EventType> TotalEvents;

	// The name and ID of the player this segment focuses on.
	std::string PlayerName;
	uint8_t PlayerID;

	// The aggregated weight of all events in this segment, determining its priority.
	int TotalScore;

	// The calculated start and end times, including pre-roll and post-roll buffers.
	float StartTime;
	float EndTime;
};



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