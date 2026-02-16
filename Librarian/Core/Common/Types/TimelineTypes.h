#pragma once

#include "Core/Common/Types/BlamTypes.h"
#include <vector>
#include <string>

// Groups different EventTypes into logical classes.
enum class EventClass
{
	Unknown,
	Fallback,
	Server,
	Match,
	Custom,
	CaptureTheFlag,
	Assault,
	Slayer,
	Juggernaut,
	Race,
	KingOfTheHill,
	Territories,
	Infection,
	Oddball,
	KillRelated
};

// Categorizes all identifiable game occurrences captured from the engine.
// These events serve as data points for the Timeline.
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

// Metadata associated with a specific game event.
struct EventInfo
{
	// The categorized type of the event used for logic identification.
	EventType Type;

	// The priority score assigned to this event.
	// Higher weights increase the likelihood of the Director focusing on this event.
	int Weight;

	// The class where this event belongs.
	EventClass Class;
};

// Simplified structure to represent team interactions within an event.
struct EventTeams
{
	// Maps to the CauseTeam (Instigator)
	int8_t First;

	// Maps to the EffectTeam (Target)
	int8_t Second;
};

// High-level representation of a player, aggregating raw engine data
// with processed metadata for UI and Director logic.
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

// A processed event entry within the AutoTheater timeline.
// Aggregates timing, types, and all participating player datafor logic evaluation.
struct GameEvent
{
	// The specific time (in seconds) the event occurred within the replay.
	float Timestamp{};

	// The categorized nature of the event (Kill, Capture, Join, etc.).
	EventType Type{};

	// Simplified team context (Cause vs. Effect team IDs).
	EventTeams Teams{};

	// List of the players involved in this specific event instance.
	std::vector<PlayerInfo> Players{};
};