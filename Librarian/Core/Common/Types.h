#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

#pragma pack(push, 1)

struct RawPlayer
{
	uint32_t SlotID;
	std::byte _pad1[36];

	uint32_t CurrentBipedHandle;
	uint32_t PreviousBipedHandle;
	std::byte _pad2[4];
	uint32_t BipedHandle;

	float Position[3];
	float Rotation[3];
	float LookVector[3];

	uint32_t hPrimaryWeapon;
	uint32_t hSecondaryWeapon;
	uint32_t hObjective;
	std::byte _pad3[72];

	wchar_t Name[28];
	std::byte _pad4[12];

	wchar_t Tag[8];
	std::byte _pad5[908];
};

struct RawBiped
{
	uint32_t ClassID;
	std::byte _pad1[28];

	float Position[3];
	std::byte _pad2[66];

	// ...
};

struct RawWeapon
{
	uint32_t ClassID;
	std::byte _pad1[8];

	uint32_t UnknownHandle2;
	std::byte _pad2[4];

	uint32_t PlayerHandle;
	std::byte _pad3[8];

	float Position[3];
	std::byte _pad4[400];

	uint32_t WeaponHandle;
	std::byte _pad5[266];

	short CurrentAmmo;
	std::byte _pad6[484];

	// ...
};

struct EventData
{
	// Player that caused the event
	uint8_t CauseSlotIndex;
	uint8_t _pad1;
	uint16_t CauseSalt;
	uint32_t CauseHandle;

	// Player that suffered the event
	uint8_t EffectSlotIndex;
	uint8_t _pad2;
	uint16_t EffectSalt;
	uint32_t EffectHandle;

	int8_t CauseTeam;
	int8_t EffectTeam;

	std::byte _pad3[10];

	uint16_t CustomValue;

	std::byte _pad4[2];
};

#pragma pack(pop)

enum Phase
{
	Default,
	BuildTimeline,
	ExecuteDirector,
};

enum class EventType
{
	Unknown, Ignore,

	// Server types
	Join, Rejoin, Quit, Booted, OneMinuteRemaining,
	OneMinuteToWin, ThirtySecondsToWin, TenSecondsRemaining,
	ThirtySecondsRemaining, JoinedTeam,

	// Match
	TookLead, TiedLead, GameOver, Wins, RoundOver, LostLead,

	// Custom
	Custom,

	// CTF
	CaptureTheFlag, FlagStolen, FlagCaptured, FlagReseted,
	FlagRecovered, FlagDropped,

	// Assault
	Assault, HasTheBomb, DroppedTheBomb, ArmedTheBomb,
	TeamScored, BombReset,

	// Slayer
	Slayer,

	// Juggernaut
	Juggernaut, IsTheJuggernaut, YouKilledTheJuggernaut,

	// Race (Should have more event types)
	Race,

	// KOTH
	KingOfTheHill, ControlsTheHill, HillContested, HillMoved,


	// Territories
	Territories, TeamCapturedATerritory, TerritoryContested,
	TerritoryLost,

	// Infection
	Infection, Infected, IsTheLastManStanding, ZombiesWin,
	SurvivorsWin, YouAreAZombie,

	// Oddball
	Oddball, PlayBall, BallReset, PickedUpTheBall, BallDropped,

	// Kill 
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

	// Keep adding game events...
};

struct EventInfo
{
	EventType Type;
	int Weight;
};

// 00 - 07
struct Teams
{
	int8_t First;
	int8_t Second;
};

struct PlayerInfo {
	RawPlayer RawPlayer{};
	std::vector<RawWeapon> Weapons{};

	std::string Name;
	std::string Tag;
	uint8_t Id = 0;
};

struct GameEvent
{
	float Timestamp;
	EventType Type;
	Teams Teams;
	std::vector<PlayerInfo> Players;
};

enum class CommandType { Cut, SetSpeed };

struct DirectorCommand
{
	float Timestamp;
	CommandType Type;

	uint8_t TargetPlayerIdx;
	std::string TargetPlayerName;

	float SpeedValue;
	std::string Reason;
};

struct ActionSegment
{
	std::vector<EventType> TotalEvents;
	std::string PlayerName;
	uint8_t PlayerID;
	int TotalScore;
	float StartTime;
	float EndTime;
};

enum class InputAction
{
	Unknown = -1,

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

struct InputRequest
{
	InputAction Action;
	InputContext Context;
};
