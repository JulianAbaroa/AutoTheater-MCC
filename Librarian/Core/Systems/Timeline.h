#pragma once

#include "Core/Systems/Theater.h"
#include <unordered_set>
#include <unordered_map>
#include <mutex>

enum class EventType
{
	Unknown, Ignore,

	// Server types
	Join, Rejoin, Quit, Booted, MinutesRemaining,
	OneMinuteToWin, ThirtySecondsToWin, TenSecondsRemaining,
	ThirtySecondsRemaining, JoinedTeam,

	// Match
	TookLead, TiedLead, GameOver, Wins, RoundOver,

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
	Inconceivable, Unfrigginbelievable, Stopped, KillWithGrenades,
	Kill, Betrayal, Suicide,

	// These doens't have a player name attached to it
	Struck, DoubleKill, TripleKill, SwordSpree, ReloadThis,
	Revenge, Killjoy, Avenger, Pull, Headcase, Assist, CloseCall,
	FirstStrike, Firebird, Protector, Overkill, HammerSpree,
	SplatterSpree, VehicularManslaughter, SundayDriver, SliceNDice,
	CuttingCrew, StickySpree, StickyFingers, Corrected, Dreamcrusher,
	WreckingCrew, ShotgunSpree, OpenSeason, BuckWild, Killtacular,
	Killtrocity, Killimanjaro, Killtastrophe, Killpocalypse, Killionaire,
	Bulltrue, KilledByTheGuardians, Yoink, 

	// Keep adding game events...
};

struct EventInfo
{
	EventType Type;
	int Weight;
};

const std::unordered_map<std::wstring, EventInfo> g_EventRegistry = {
	// Server
	{ L"#cause_player joined the game",								{ EventType::Join,					0 } },
	{ L"#cause_player quit",										{ EventType::Quit,					0 } },
	{ L"#effect_player was booted",									{ EventType::Booted,				0 } },
	{ L"#cause_player joined the #effect_team",						{ EventType::JoinedTeam,			0 } },
	{ L"#cause_player rejoined the game",							{ EventType::Rejoin,				0 } },

	// Match
	{ L"#cause_team took the lead!",								{ EventType::TookLead,				0 } },
	{ L"#cause_team tied for the lead!",							{ EventType::TiedLead,				0 } },
	{ L"Round over",												{ EventType::RoundOver,				0 } },
	{ L"Game over",													{ EventType::GameOver,				0 } },

	// CTF
	{ L"Capture the Flag",											{ EventType::CaptureTheFlag,		0 } },
	{ L"#cause_team captured a flag!",								{ EventType::FlagCaptured,			100 } },
	{ L"#effect_team flag was stolen!",								{ EventType::FlagStolen,			75 } },
	{ L"#effect_team flag dropped",									{ EventType::FlagDropped,			0 } },
	{ L"#effect_team flag reset",									{ EventType::FlagReseted,			0 } },
	{ L"#effect_team flag recovered",								{ EventType::FlagRecovered,			0 } },

	// Slayer
	{ L"Slayer",													{ EventType::Slayer,				0 } },

	// Kill
	{ L"Killionaire!",												{ EventType::Killionaire,			100 } },
	{ L"Killpocalypse!",											{ EventType::Killpocalypse,			90 } },
	{ L"Killtastrophe!",											{ EventType::Killtastrophe,			80 } },
	{ L"Killimanjaro!",												{ EventType::Killimanjaro,			70 } },
	{ L"Killtrocity!",												{ EventType::Killtrocity,			60 } },
	{ L"Killtacular!",												{ EventType::Killtacular,			50 } },
	{ L"#cause_player stopped #effect_player",						{ EventType::Stopped,				50 } },
	{ L"Overkill!",													{ EventType::Overkill,				40 } },
	{ L"#cause_player stuck #effect_player",						{ EventType::Stuck,					30 } },
	{ L"#cause_player assassinated #effect_player",					{ EventType::Assassinated,			30 } },
	{ L"#effect_player was assassinated by #cause_player",			{ EventType::Assassinated,			30 } },
	{ L"Yoink!",													{ EventType::Yoink,					30 } },
	{ L"Triple Kill!",												{ EventType::TripleKill,			30 } },
	{ L"Double Kill!",												{ EventType::DoubleKill,			20 } },
	{ L"#cause_player beat down #effect_player",					{ EventType::BeatDown,				20 } },
	{ L"Headcase!",													{ EventType::Headcase,				20 } },
	{ L"Splatter Spree!",											{ EventType::SplatterSpree,			10 } },
	{ L"Vehicular Manslaughter!",									{ EventType::VehicularManslaughter,	10 } },
	{ L"Sunday Driver!",											{ EventType::SundayDriver,			10 } },
	{ L"Sticky Spree!",												{ EventType::StickySpree,			10 } },
	{ L"Sticky Fingers!",											{ EventType::StickyFingers,			10 } },
	{ L"Corrected!",												{ EventType::Corrected,				10 } },
	{ L"Shotgun Spree!",											{ EventType::ShotgunSpree,			10 } },
	{ L"Open Season!",												{ EventType::OpenSeason,			10 } },
	{ L"Buck Wild!",												{ EventType::BuckWild,				10 } },
	{ L"Wrecking Crew!",											{ EventType::WreckingCrew,			10 } },
	{ L"Dreamcrusher!",												{ EventType::Dreamcrusher,			10 } },
	{ L"Hammer Spree!",												{ EventType::HammerSpree,			10 } },
	{ L"Sword Spree!",												{ EventType::SwordSpree,			10 } },
	{ L"Slice 'n Dice!",											{ EventType::SliceNDice,			10 } },
	{ L"#cause_player is on a Killing Spree!",						{ EventType::KillingSpree,			10 } },
	{ L"Killing Spree!",											{ EventType::KillingSpree,			10 } },
	{ L"#cause_player is on a Killing Frenzy!",						{ EventType::KillingFrenzy,			10 } },
	{ L"Killing Frenzy!",											{ EventType::KillingFrenzy,			10 } },
	{ L"#cause_player is a Running Riot!",							{ EventType::RunningRiot,			10 } },
	{ L"Running Riot!",												{ EventType::RunningRiot,			10 } },
	{ L"#cause_player is on a Rampage!",							{ EventType::Rampage,				10 } },
	{ L"#cause_player is Untouchable!",								{ EventType::Untouchable,			10 } },
	{ L"#cause_player is Invincible!",								{ EventType::Invincible,			10 } },
	{ L"#cause_player is Inconceivable!",							{ EventType::Inconceivable,			10 } },
	{ L"#cause_player is Unfrigginbelievable!",						{ EventType::Unfrigginbelievable,	10 } },
	{ L"#cause_player splattered #effect_player",					{ EventType::Splattered,			10 } },
	{ L"#cause_player pummeled #effect_player",						{ EventType::Pummeled,				10 } },
	{ L"#cause_player killed #effect_player with a headshot",		{ EventType::Headshot,				10 } },
	{ L"#effect_player was killed by a #cause_player headshot",		{ EventType::Headshot,				10 } },
	{ L"#cause_player got revenge!",								{ EventType::Revenge,				5 } },
	{ L"Revenge!",													{ EventType::Revenge,				5 } },
	{ L"First Strike!",												{ EventType::FirstStrike,			5 } },
	{ L"Reload This!",												{ EventType::ReloadThis,			5 } },
	{ L"Close Call!",												{ EventType::CloseCall,				5 } },
	{ L"Protector!",												{ EventType::Protector,				5 } },
	{ L"Firebird!",													{ EventType::Firebird,				5 } },
	{ L"Killjoy!",													{ EventType::Killjoy,				5 } },
	{ L"Avenger!",													{ EventType::Avenger,				5 } },
	{ L"Pull!",														{ EventType::Pull,					5 } },
	{ L"#cause_player struck #effect_player down!",					{ EventType::Struck,				5 } },
	{ L"#cause_player killed #effect_player with grenades",			{ EventType::KillWithGrenades,		5 } },
	{ L"#effect_player was killed by #cause_player with grenades",	{ EventType::KillWithGrenades,		5 } },
	{ L"#cause_player lasered #effect_player",						{ EventType::Lasered,				5 } },
	{ L"#effect_player was lasered by #cause_player",				{ EventType::Lasered,				5 } },
	{ L"#cause_player sniped #effect_player",						{ EventType::Sniped,				5 } },
	{ L"Assist",													{ EventType::Assist,				0 } },
	{ L"#cause_player killed #effect_player",						{ EventType::Kill,					0 } },
	{ L"#cause_player committed suicide",							{ EventType::Suicide,				0 } },
	{ L"#cause_player betrayed #effect_player",						{ EventType::Betrayal,				0 } },
	{ L"Killed by the Guardians",									{ EventType::KilledByTheGuardians,	0 } },

	// Ignore
	{ L"You killed #effect_player",									{ EventType::Ignore,				0 } },
	{ L"#cause_player killed you",									{ EventType::Ignore,				0 } },
	{ L"#cause_player pummeled you",								{ EventType::Ignore,				0 } },
	{ L"You pummeled #effect_player",								{ EventType::Ignore,				0 } },
	{ L"Your flag has been stolen!",								{ EventType::Ignore,				0 } },
	{ L"You grabbed the #effect_team flag!",						{ EventType::Ignore,				0 } },
	{ L"Your flag has been stolen!",								{ EventType::Ignore,				0 } },
	{ L"You dropped #effect_team flag",								{ EventType::Ignore,				0 } },
	{ L"You splattered #effect_player",								{ EventType::Ignore,				0 } },
	{ L"#cause_player splattered you",								{ EventType::Ignore,				0 } },
	{ L"You stopped #effect_player",								{ EventType::Ignore,				0 } },
	{ L"#cause_player stopped you",									{ EventType::Ignore,				0 } },
	{ L"You killed #effect_player with grenades",					{ EventType::Ignore,				0 } },
	{ L"#cause_player killed you with grenades",					{ EventType::Ignore,				0 } },
	{ L"#cause_player beat you down",								{ EventType::Ignore,				0 } },
	{ L"You beat down #effect_player",								{ EventType::Ignore,				0 } },
	{ L"#cause_player killed you with a headshot",					{ EventType::Ignore,				0 } },
	{ L"You killed #effect_player with a headshot",					{ EventType::Ignore,				0 } },
	{ L"#cause_player ended your spree",							{ EventType::Ignore,				0 } },
	{ L"Your flag has been recovered",								{ EventType::Ignore,				0 } },
	{ L"You committed suicide",										{ EventType::Ignore,				0 } },
	{ L"#cause_player killed you from the grave",					{ EventType::Ignore,				0 } },
	{ L"You killed #effect_player from the grave",					{ EventType::Ignore,				0 } },
	{ L"#cause_player stuck you" ,									{ EventType::Ignore,				0 } },
	{ L"You stuck #effect_player",									{ EventType::Ignore,				0 } },
	{ L"#cause_player assassinated you",							{ EventType::Ignore,				0 } },
	{ L"You assassinated #effect_player",							{ EventType::Ignore,				0 } },
	{ L"You captured a flag!",										{ EventType::Ignore,				0 } },
	{ L"#cause_player joined your team",							{ EventType::Ignore,				0 } },
	{ L"You joined the #effect_team",								{ EventType::Ignore,				0 } },
	{ L"#effect_player avenged your death.",						{ EventType::Ignore,				0 } },
	{ L"#cause_player struck you down!",							{ EventType::Ignore,				0 } },
	{ L"Your team lost the lead",									{ EventType::Ignore,				0 } },
	{ L"Your team took the lead!",									{ EventType::Ignore,				0 } },
	{ L"Your team is tied for the lead!",							{ EventType::Ignore,				0 } },
	{ L"You betrayed #effect_player",								{ EventType::Ignore,				0 } },
	{ L"#cause_player betrayed you",								{ EventType::Ignore,				0 } },
	{ L"You were killed while reloading",							{ EventType::Ignore,				0 } },
	{ L"You fell to your death",									{ EventType::Ignore,				0 } },
	{ L"#cause_player sniped you",									{ EventType::Ignore,				0 } },
	{ L"You sniped #effect_player",									{ EventType::Ignore,				0 } },
	{ L"You lasered #effect_player",								{ EventType::Ignore,				0 } },
	{ L"#cause_player lasered you",									{ EventType::Ignore,				0 } },
};

// TODO: Seguir completando estos eventos
//const std::map<EventType, EventInfo> g_EventRegistry = {
//	// Server
//	{ EventType::MinutesRemaining,		{ L"minutesremaining",		0 } },
//	{ EventType::TenSecondsRemaining,	{ L"10secondsremaining",	0 } },
//	{ EventType::ThirtySecondsRemaining,{ L"30secondsremaining",	0 } },
//
//	// Match
//	{ EventType::Wins,					{ L"wins",					100 } },
//
//	// Assault
//	{ EventType::Assault,				{ L"assault",				0 } },
//	{ EventType::HasTheBomb,			{ L"hasthebomb",			75 } },
//	{ EventType::DroppedTheBomb,		{ L"droppedthebomb",		0 } },
//	{ EventType::ArmedTheBomb,			{ L"armedthebomb",			100 } },
//	{ EventType::TeamScored,			{ L"teamscored",			0 } },
//	{ EventType::BombReset,				{ L"bombreset",				0 } },
//
//	// Juggernaut
//	{ EventType::Juggernaut,			{ L"juggernaut",			0 } },
//	{ EventType::IsTheJuggernaut,		{ L"isthejuggernaut",		75 } },
//
//	// Race 
//	{ EventType::Race,					{ L"race",					0 } },
//
//	// KOTH
//	{ EventType::KingOfTheHill,			{ L"kingofthehill",			0 } },
//	{ EventType::ControlsTheHill,		{ L"controlsthehill",		75 } },
//	{ EventType::HillContested,			{ L"hillcontested",			0 } },
//	{ EventType::OneMinuteToWin,		{ L"1minutetowin",			75 } },
//	{ EventType::ThirtySecondsToWin,	{ L"30secondstowin",		75 } },
//
//	// Territories
//	{ EventType::Territories,			{ L"territories",			0 } },
//	{ EventType::TeamCapturedATerritory,{ L"teamcapturedaterritory",0 } },
//	{ EventType::TerritoryContested,	{ L"territorycontested",	0 } },
//	{ EventType::TerritoryLost,			{ L"territorylost",			0 } },
//
//	// Infection
//	{ EventType::Infection,				{ L"infection",				0 } },
//	{ EventType::Infected,				{ L"infected",				75 } },
//	{ EventType::IsTheLastManStanding,	{ L"isthelastmanstanding",	100 } },
//	{ EventType::ZombiesWin,			{ L"zombieswin",			0 } },
//	{ EventType::SurvivorsWin,			{ L"survivorswin",			0 } },
//
// 	// Oddball
//	{ EventType::Oddball,				{ L"oddball",				0 } },
//	{ EventType::PlayBall,				{ L"playball",				0 } },
//	{ EventType::BallReset,				{ L"ballreset",				0 } },
//	{ EventType::PickedUpTheBall,		{ L"pickeduptheball",		75 } },
//	{ EventType::BallDropped,			{ L"balldropped",			0 } },
//
//	{ EventType::Bulltrue,				{ L"bulltrue",				20 } }, 
//	{ EventType::CuttingCrew,			{ L"cuttingcrew",			10 } }, 

const std::unordered_set<EventType> g_OrphanEvents = {
	EventType::Struck, EventType::DoubleKill, EventType::TripleKill,
	EventType::SwordSpree, EventType::ReloadThis, EventType::Revenge,
	EventType::Killjoy, EventType::Avenger, EventType::Pull,
	EventType::Headcase, /*EventType::Assist,*/ EventType::CloseCall,
	EventType::FirstStrike, EventType::Firebird, EventType::Protector,
	EventType::Overkill, EventType::Revenge, EventType::HammerSpree,
	EventType::KillingSpree
};

// 00 - 07
struct Teams 
{
	uint8_t First;
	uint8_t Second;
};

struct GameEvent
{
	float Timestamp;
	EventType Type;
	Teams Teams;
	std::vector<PlayerInfo> Players;
};

extern std::vector<GameEvent> g_Timeline;
extern std::mutex g_TimelineMutex;

extern bool g_IsLastEvent;

namespace Timeline
{
	void AddGameEvent(
		float timestamp, 
		std::wstring& templateStr,
		EventData* eventData
	);
};