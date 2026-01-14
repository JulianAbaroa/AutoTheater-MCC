#pragma once

#include "Core/Systems/Theater.h"
#include <unordered_set>
#include <map>

struct EventInfo
{
	std::wstring Keyword;
	int Weight;
};

enum class EventType
{
	Unknown,

	// Server types
	Join, Rejoin, Quit, Booted, MinutesRemaining,
	OneMinuteToWin, ThirtySecondsToWin, TenSecondsRemaining,
	ThirtySecondsRemaining,

	// Match
	TookLead, TiedLead, GameOver, Wins,

	// Death types
	Kill, Betrayal, Suicide,

	// CTF
	CaptureTheFlag, FlagStolen, FlagCaptured, FlagReseted,
	FlagRecovered, FlagDropped, 

	// Assault
	Assault, HasTheBomb, DroppedTheBomb, ArmedTheBomb,
	TeamScored, BombReset,

	// Slayer
	Slayer,

	// Juggernaut
	Juggernaut, IsTheJuggernaut,

	// Race (Should have more event types)
	Race,

	// KOTH
	KingOfTheHill, ControlsTheHill, HillContested,
	

	// Territories
	Territories, TeamCapturedATerritory, TerritoryContested,
	TerritoryLost,

	// Infection
	Infection, Infected, IsTheLastManStanding, ZombiesWin,
	SurvivorsWin,

	// Oddball
	Oddball, PlayBall, BallReset, PickedUpTheBall, BallDropped,

	// Kill types
	Headshot, Pummeled, Stuck, KillingSpree, Splattered, 
	BeatDown, Assassinated, KillingFrenzy, RunningRiot, 
	Lasered, Sniped, Rampage, Untouchable, Invincible, 
	Inconceivable, Unfrigginbelievable, Stopped,

	// These doens't have a player name attached to it
	Struck, DoubleKill, TripleKill, SwordSpree, ReloadThis,
	Revenge, Killjoy, Avenger, Pull, Headcase, Assist, CloseCall,
	FirstStrike, Firebird, Protector, Overkill, HammerSpree,
	SplatterSpree, VehicularManslaughter, SundayDriver, SliceNDice,
	CuttingCrew, StickySpree, StickyFingers, Corrected, Dreamcrusher,
	WreckingCrew, ShotgunSpree, OpenSeason, BuckWild, Killtacular,
	Killtrocity, Killimanjaro, Killtastrophe, Killpocalypse, Killionaire,
	Bulltrue,

	// Keep adding game events...
};

// TODO: SEE IF ALL THE EVENTS ARE CORRECTLY PARSED
const std::map<EventType, EventInfo> g_EventRegistry = {
	// Server
	{ EventType::Join,					{ L"joined",				0 } },
	{ EventType::Rejoin,				{ L"rejoined",				0 } },
	{ EventType::Quit,					{ L"quit",					0 } },
	{ EventType::Booted,				{ L"booted",				0 } },
	{ EventType::MinutesRemaining,		{ L"minutesremaining",		0 } },
	{ EventType::TenSecondsRemaining,	{ L"10secondsremaining",	0 } },
	{ EventType::ThirtySecondsRemaining,{ L"30secondsremaining",	0 } },

	// Match
	{ EventType::TookLead,				{ L"tookthelead",			0 } },
	{ EventType::TiedLead,				{ L"tiedforthelead",		0 } },
	{ EventType::GameOver,				{ L"gameover",				0 } },
	{ EventType::Wins,					{ L"wins",					100 } },

	// CTF
	{ EventType::CaptureTheFlag,		{ L"capturetheflag",		0 } },
	{ EventType::FlagCaptured,			{ L"youcapturedaflag",		100 } },
	{ EventType::FlagStolen,			{ L"yougrabbedthe",			75 } },
	{ EventType::FlagReseted,			{ L"teamflagreset",			0 } },
	{ EventType::FlagRecovered,			{ L"teamflagrecovered",		0 } },
	{ EventType::FlagDropped,			{ L"flagdropped",			0 } },	

	// Assault
	{ EventType::Assault,				{ L"assault",				0 } },
	{ EventType::HasTheBomb,			{ L"hasthebomb",			75 } },
	{ EventType::DroppedTheBomb,		{ L"droppedthebomb",		0 } },
	{ EventType::ArmedTheBomb,			{ L"armedthebomb",			100 } },
	{ EventType::TeamScored,			{ L"teamscored",			0 } },
	{ EventType::BombReset,				{ L"bombreset",				0 } },

	// Slayer
	{ EventType::Slayer,				{ L"slayer",				0 } },

	// Juggernaut
	{ EventType::Juggernaut,			{ L"juggernaut",			0 } },
	{ EventType::IsTheJuggernaut,		{ L"isthejuggernaut",		75 } },

	// Race 
	{ EventType::Race,					{ L"race",					0 } },

	// KOTH
	{ EventType::KingOfTheHill,			{ L"kingofthehill",			0 } },
	{ EventType::ControlsTheHill,		{ L"controlsthehill",		75 } },
	{ EventType::HillContested,			{ L"hillcontested",			0 } },
	{ EventType::OneMinuteToWin,		{ L"1minutetowin",			75 } },
	{ EventType::ThirtySecondsToWin,	{ L"30secondstowin",		75 } },

	// Territories
	{ EventType::Territories,			{ L"territories",			0 } },
	{ EventType::TeamCapturedATerritory,{ L"teamcapturedaterritory",0 } },
	{ EventType::TerritoryContested,	{ L"territorycontested",	0 } },
	{ EventType::TerritoryLost,			{ L"territorylost",			0 } },

	// Infection
	{ EventType::Infection,				{ L"infection",				0 } },
	{ EventType::Infected,				{ L"infected",				75 } },
	{ EventType::IsTheLastManStanding,	{ L"isthelastmanstanding",	100 } },
	{ EventType::ZombiesWin,			{ L"zombieswin",			0 } },
	{ EventType::SurvivorsWin,			{ L"survivorswin",			0 } },

	// Oddball
	{ EventType::Oddball,				{ L"oddball",				0 } },
	{ EventType::PlayBall,				{ L"playball",				0 } },
	{ EventType::BallReset,				{ L"ballreset",				0 } },
	{ EventType::PickedUpTheBall,		{ L"pickeduptheball",		75 } },
	{ EventType::BallDropped,			{ L"balldropped",			0 } },

	// Kill
	{ EventType::Killionaire,			{ L"killionaire",			100 } },
	{ EventType::Killpocalypse,			{ L"killpocalypse",			90 } },
	{ EventType::Killtastrophe,			{ L"killtastrophe",			80 } },
	{ EventType::Killimanjaro,			{ L"killimanjaro",			70 } },
	{ EventType::Killtrocity,			{ L"killtrocity",			60 } },
	{ EventType::Killtacular,			{ L"killtacular",			50 } },
	{ EventType::Stopped,				{ L"stopped",				50 } },
	{ EventType::Overkill,				{ L"overkill",				40 } },
	{ EventType::Stuck,					{ L"stuck",					30 } },
	{ EventType::Assassinated,			{ L"assassinated",			30 } },
	{ EventType::TripleKill,			{ L"triplekill",			30 } },
	{ EventType::BeatDown,				{ L"beatdown",				20 } },
	{ EventType::Headcase,				{ L"headcase",				20 } },
	{ EventType::DoubleKill,			{ L"doublekill",			20 } },
	{ EventType::Bulltrue,				{ L"bulltrue",				20 } },
	{ EventType::Splattered,			{ L"splattered",			10 } },
	{ EventType::Headshot,				{ L"headshot",				10 } },
	{ EventType::Pummeled,				{ L"pummeled",				10 } },
	{ EventType::KillingSpree,			{ L"killingspree",			10 } },
	{ EventType::SwordSpree,			{ L"swordspree",			10 } },
	{ EventType::HammerSpree,			{ L"hammerspree",			10 } },
	{ EventType::KillingFrenzy,			{ L"killingfrenzy",			10 } },
	{ EventType::RunningRiot,			{ L"runningriot",			10 } },
	{ EventType::Rampage,				{ L"rampage",				10 } },
	{ EventType::SplatterSpree,			{ L"splatterspree",			10 } },
	{ EventType::VehicularManslaughter,	{ L"vehicularmanslaughter",	10 } },
	{ EventType::SundayDriver,			{ L"sundaydriver",			10 } },
	{ EventType::SliceNDice,			{ L"slice'ndice",			10 } },
	{ EventType::CuttingCrew,			{ L"cuttingcrew",			10 } },
	{ EventType::StickySpree,			{ L"stickyspree",			10 } },
	{ EventType::StickyFingers,			{ L"stickyfingers",			10 } },
	{ EventType::Corrected,				{ L"corrected",				10 } },
	{ EventType::Dreamcrusher,			{ L"dreamcrusher",			10 } },
	{ EventType::WreckingCrew,			{ L"wreckingcrew",			10 } },
	{ EventType::ShotgunSpree,			{ L"shotgunspree",			10 } },
	{ EventType::OpenSeason,			{ L"openseason",			10 } },
	{ EventType::BuckWild,				{ L"buckwild",				10 } },
	{ EventType::Untouchable,			{ L"untouchable",			10 } },
	{ EventType::Invincible,			{ L"invincible",			10 } },
	{ EventType::Inconceivable,			{ L"inconceivable",			10 } },
	{ EventType::Unfrigginbelievable,	{ L"unfrigginbelievable",	10 } },
	{ EventType::Struck,				{ L"struck",				5  } },
	{ EventType::ReloadThis,			{ L"reloadthis",			5 } },
	{ EventType::Revenge,				{ L"revenge",				5 } },
	{ EventType::Killjoy,				{ L"killjoy",				5 } },
	{ EventType::Avenger,				{ L"avenger",				5 } },
	{ EventType::Pull,					{ L"pull",					5 } },
	{ EventType::CloseCall,				{ L"closecall",				5 } },
	{ EventType::FirstStrike,			{ L"firststrike",			5 } },
	{ EventType::Firebird,				{ L"firebird",				5 } },
	{ EventType::Protector,				{ L"protector",				5 } },
	{ EventType::Lasered,				{ L"lasered",				5 } },
	{ EventType::Sniped,				{ L"sniped",				5 } },
	{ EventType::Assist,				{ L"assist",				0 } },
	{ EventType::Kill,					{ L"killed",				0 } },
	{ EventType::Betrayal,				{ L"betrayed",				0 } },
	{ EventType::Suicide,				{ L"suicide",				0 } },
};

const std::unordered_set<EventType> g_OrphanEvents = {
	EventType::Struck, EventType::DoubleKill, EventType::TripleKill,
	EventType::SwordSpree, EventType::ReloadThis, EventType::Revenge,
	EventType::Killjoy, EventType::Avenger, EventType::Pull,
	EventType::Headcase, /*EventType::Assist,*/ EventType::CloseCall,
	EventType::FirstStrike, EventType::Firebird, EventType::Protector,
	EventType::Overkill, EventType::Revenge, EventType::HammerSpree,
	EventType::KillingSpree
};

struct GameEvent
{
	float Timestamp;
	EventType Type;
	std::vector<PlayerInfo> Players;
};

extern std::vector<GameEvent> g_Timeline;
extern bool g_IsLastEvent;

namespace Timeline
{
	void AddGameEvent(
		float timestamp, 
		std::wstring eventText, 
		unsigned int playerHandle
	);
};