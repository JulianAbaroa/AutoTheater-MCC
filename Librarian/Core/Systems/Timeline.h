#pragma once

#include "Core/Systems/Theater.h"
#include <unordered_set>
#include <unordered_map>
#include <mutex>

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


// TODO: Add all of them
const std::unordered_map<std::wstring, EventInfo> g_EventRegistry = {
	// Server
	{ L"#cause_player joined the game",								{ EventType::Join,					0 } },
	{ L"#cause_player rejoined the game",							{ EventType::Rejoin,				0 } },
	{ L"#cause_player quit",										{ EventType::Quit,					0 } },
	{ L"#effect_player was booted",									{ EventType::Booted,				0 } },
	{ L"#cause_player joined the #effect_team",						{ EventType::JoinedTeam,			0 } },
	{ L"#cause_player joined your team",							{ EventType::JoinedTeam,			0 } },
	{ L"You joined the #effect_team",								{ EventType::JoinedTeam,			0 } },

	// Match
	{ L"#cause_team took the lead!",								{ EventType::TookLead,				0 } },
	{ L"#cause_player took the lead!",								{ EventType::TookLead,				0 } },
	{ L"Your team took the lead!",									{ EventType::TookLead,				0 } },
	{ L"You took the lead!",										{ EventType::TookLead,				0 } },
	{ L"You lost the lead",											{ EventType::LostLead,				0 } },
	{ L"Your team lost the lead",									{ EventType::LostLead,				0 } },
	{ L"#cause_team tied for the lead!",							{ EventType::TiedLead,				0 } },
	{ L"You are tied for the lead!",								{ EventType::TiedLead,				0 } },
	{ L"Your team is tied for the lead!",							{ EventType::TiedLead,				0 } },
	{ L"#cause_player tied for the lead!",							{ EventType::TiedLead,				0 } },
	{ L"1 minute to win!",											{ EventType::OneMinuteToWin,		75 } },
	{ L"#cause_player 1 minute to win!",							{ EventType::OneMinuteToWin,		75 } },
	{ L"30 seconds to win!",										{ EventType::ThirtySecondsToWin,	75 } },
	{ L"#cause_player 30 seconds to win!",							{ EventType::ThirtySecondsToWin,	75 } },
	{ L"1 minute remaining",										{ EventType::OneMinuteRemaining,	0 } },
	{ L"30 seconds remaining",										{ EventType::TenSecondsRemaining,	0 } },
	{ L"10 seconds remaining",										{ EventType::ThirtySecondsRemaining,0 } },
	{ L"Round over",												{ EventType::RoundOver,				0 } },
	{ L"Game over",													{ EventType::GameOver,				0 } },

	// Custom
	{ L"Custom",													{ EventType::Custom,				0 } },

	// CTF
	{ L"Capture the Flag",											{ EventType::CaptureTheFlag,		0 } },
	{ L"#cause_team captured a flag!",								{ EventType::FlagCaptured,			100 } },
	{ L"You captured a flag!",										{ EventType::FlagCaptured,			100 } },
	{ L"#effect_team flag was stolen!",								{ EventType::FlagStolen,			75 } },
	{ L"Your flag has been stolen!",								{ EventType::FlagStolen,			75 } }, // check this two
	{ L"You grabbed the #effect_team flag!",						{ EventType::FlagStolen,			75 } }, // check this two
	{ L"#effect_team flag dropped",									{ EventType::FlagDropped,			0 } },
	{ L"You dropped #effect_team flag",								{ EventType::FlagDropped,			0 } },
	{ L"#effect_team flag reset",									{ EventType::FlagReseted,			0 } },
	{ L"#effect_team flag recovered",								{ EventType::FlagRecovered,			0 } },
	{ L"Your flag has been recovered",								{ EventType::FlagRecovered,			0 } },

	// Slayer
	{ L"Slayer",													{ EventType::Slayer,				0 } },

	// Race 
	{ L"Race",														{ EventType::Race,					0 } },

	// Assault
	{ L"Assault",													{ EventType::Assault,				0 } },
	{ L"You picked up the bomb", 									{ EventType::HasTheBomb,			75 } },
	{ L"You armed the bomb",										{ EventType::ArmedTheBomb,			100 } },
	{ L"#cause_player armed the bomb",								{ EventType::ArmedTheBomb,			100 } },
	{ L"You dropped the bomb",										{ EventType::DroppedTheBomb,		0 } },
	{ L"#cause_team scored", 										{ EventType::TeamScored,			0 } },
	{ L"Bomb reset",												{ EventType::BombReset,				0 } },

	// Juggernaut
	{ L"Juggernaut",												{ EventType::Juggernaut,			0 } },
	{ L"You are the juggernaut",									{ EventType::IsTheJuggernaut,		75 } },
	{ L"#cause_player is the juggernaut",							{ EventType::IsTheJuggernaut,		75 } },
	{ L"You killed the juggernaut",									{ EventType::YouKilledTheJuggernaut,75 } },

	// KOTH
	{ L"King of the Hill",											{ EventType::KingOfTheHill,			0 } },
	{ L"#cause_player controls the hill", 							{ EventType::ControlsTheHill,		75 } },
	{ L"You control the hill", 										{ EventType::ControlsTheHill,		75 } },
	{ L"Your team controls the hill", 								{ EventType::ControlsTheHill,		75 } },
	{ L"Hill contested",											{ EventType::HillContested,			0 } },
	{ L"Hill moved",												{ EventType::HillMoved,				0 } },

	// Territories
	{ L"Territories",												{ EventType::Territories,			0 } },
	{ L"#cause_team captured a territory",							{ EventType::TeamCapturedATerritory,0 } },
	{ L"Your team captured a territory",							{ EventType::TeamCapturedATerritory,0 } },
	{ L"Territory contested",										{ EventType::TerritoryContested,	0 } },
	{ L"Territory lost",											{ EventType::TerritoryLost,			0 } },

	// Infection
	{ L"Infection",													{ EventType::Infection,				0 } },
	{ L"You are a zombie",											{ EventType::YouAreAZombie,			0 } },
	{ L"You infected #effect_player",								{ EventType::Infected,				75 } },
	{ L"#cause_player infected you",								{ EventType::Infected,				75 } },
	{ L"#cause_player is the last man standing",					{ EventType::IsTheLastManStanding,	0 } },
	{ L"You are the last man standing",								{ EventType::IsTheLastManStanding,	0 } },
	{ L"Zombies Win!",												{ EventType::ZombiesWin,			0 } },
	{ L"Survivors Win!",											{ EventType::SurvivorsWin,			0 } },
	
	// Oddball
	{ L"Oddball",													{ EventType::Oddball,				0 } },
	{ L"Play ball!",												{ EventType::PlayBall,				0 } },
	{ L"#cause_player picked up the ball",							{ EventType::PickedUpTheBall,		75 } },
	{ L"You picked up the ball",									{ EventType::PickedUpTheBall,		75 } },
	{ L"Ball dropped",												{ EventType::BallDropped,			0 } },
	{ L"You dropped the ball",										{ EventType::BallDropped,			0 } },
	{ L"Ball reset",												{ EventType::BallReset,				0 } },
	
	// Kill
	{ L"Killionaire!",												{ EventType::Killionaire,			100 } },
	{ L"Killpocalypse!",											{ EventType::Killpocalypse,			90 } },
	{ L"Killtastrophe!",											{ EventType::Killtastrophe,			80 } },
	{ L"Killimanjaro!",												{ EventType::Killimanjaro,			70 } },
	{ L"Killtrocity!",												{ EventType::Killtrocity,			60 } },
	{ L"Killtacular!",												{ EventType::Killtacular,			50 } },
	{ L"#cause_player stopped #effect_player",						{ EventType::Stopped,				50 } },
	{ L"You stopped #effect_player",								{ EventType::Stopped,				50 } }, // check this two
	{ L"#cause_player stopped you",									{ EventType::Stopped,				50 } }, // check this two
	{ L"Overkill!",													{ EventType::Overkill,				40 } },
	{ L"#cause_player stuck #effect_player",						{ EventType::Stuck,					30 } },
	{ L"#cause_player stuck you" ,									{ EventType::Stuck,					30 } },
	{ L"You stuck #effect_player",									{ EventType::Stuck,					30 } },
	{ L"#cause_player assassinated #effect_player",					{ EventType::Assassinated,			30 } },
	{ L"#effect_player was assassinated by #cause_player",			{ EventType::Assassinated,			30 } },
	{ L"#cause_player assassinated you",							{ EventType::Assassinated,			30 } },
	{ L"You assassinated #effect_player",							{ EventType::Assassinated,			30 } },
	{ L"You were killed while performing an assassination.",		{ EventType::ShowStopper,			30 } },
	{ L"Showstopper!",												{ EventType::ShowStopper,			30 } },
	{ L"Yoink!",													{ EventType::Yoink,					30 } },
	{ L"Triple Kill!",												{ EventType::TripleKill,			30 } },
	{ L"Double Kill!",												{ EventType::DoubleKill,			20 } },
	{ L"#cause_player beat down #effect_player",					{ EventType::BeatDown,				20 } },
	{ L"#cause_player beat you down",								{ EventType::BeatDown,				20 } },
	{ L"You beat down #effect_player",								{ EventType::BeatDown,				20 } },
	{ L"Headcase!",													{ EventType::Headcase,				20 } },
	{ L"Bulltrue!",													{ EventType::Bulltrue,				20 } },
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
	{ L"Cutting Crew!", 											{ EventType::CuttingCrew,			10 } },

	// TODO: Get the other ones
	{ L"Juggernaut Spree!",											{ EventType::JuggernautSpree,		10 } },

	{ L"#cause_player's driving assisted you",						{ EventType::Wheelman,				10 } },
	{ L"Your driving assisted #effect_player",						{ EventType::Wheelman,				10 } },
	{ L"#cause_player is on a Killing Spree!",						{ EventType::KillingSpree,			10 } },
	{ L"Killing Spree!",											{ EventType::KillingSpree,			10 } },
	{ L"#cause_player is on a Killing Frenzy!",						{ EventType::KillingFrenzy,			10 } },
	{ L"Killing Frenzy!",											{ EventType::KillingFrenzy,			10 } },
	{ L"#cause_player is a Running Riot!",							{ EventType::RunningRiot,			10 } },
	{ L"Running Riot!",												{ EventType::RunningRiot,			10 } },
	{ L"#cause_player is on a Rampage!",							{ EventType::Rampage,				10 } },
	{ L"Rampage!",													{ EventType::Rampage,				10 } },
	{ L"#cause_player is Untouchable!",								{ EventType::Untouchable,			10 } },
	{ L"Untouchable!",												{ EventType::Untouchable,			10 } },
	{ L"#cause_player is Invincible!",								{ EventType::Invincible,			10 } },
	{ L"Invincible!",												{ EventType::Invincible,			10 } },
	{ L"#cause_player is Inconceivable!",							{ EventType::Inconceivable,			10 } },
	{ L"Inconceivable!",											{ EventType::Inconceivable,			10 } },
	{ L"#cause_player is Unfrigginbelievable!",						{ EventType::Unfrigginbelievable,	10 } },
	{ L"Unfrigginbelievable!",										{ EventType::Unfrigginbelievable,	10 } },
	{ L"#cause_player splattered #effect_player",					{ EventType::Splattered,			10 } },
	{ L"You splattered #effect_player",								{ EventType::Splattered,			10 } },
	{ L"#cause_player splattered you",								{ EventType::Splattered,			10 } },
	{ L"#cause_player pummeled #effect_player",						{ EventType::Pummeled,				10 } },
	{ L"#cause_player pummeled you",								{ EventType::Pummeled,				10 } },
	{ L"You pummeled #effect_player",								{ EventType::Pummeled,				10 } },
	{ L"#cause_player killed #effect_player with a headshot",		{ EventType::Headshot,				10 } },
	{ L"#effect_player was killed by a #cause_player headshot",		{ EventType::Headshot,				10 } },
	{ L"#cause_player killed you with a headshot",					{ EventType::Headshot,				10 } },
	{ L"You killed #effect_player with a headshot",					{ EventType::Headshot,				10 } },
	{ L"#cause_player killed you from the grave",					{ EventType::KillFromTheGrave,		5 } },
	{ L"You killed #effect_player from the grave",					{ EventType::KillFromTheGrave,		5 } },
	{ L"#cause_player got revenge!",								{ EventType::Revenge,				5 } },
	{ L"Revenge!",													{ EventType::Revenge,				5 } },
	{ L"First Strike!",												{ EventType::FirstStrike,			5 } },
	{ L"Reload This!",												{ EventType::ReloadThis,			5 } },
	{ L"You were killed while reloading",							{ EventType::ReloadThis,			5 } },
	{ L"Close Call!",												{ EventType::CloseCall,				5 } },
	{ L"Protector!",												{ EventType::Protector,				5 } },
	{ L"#effect_player saved your life.",							{ EventType::Protector,				5 } },
	{ L"Firebird!",													{ EventType::Firebird,				5 } },
	{ L"Killjoy!",													{ EventType::Killjoy,				5 } },
	{ L"#cause_player ended your spree",							{ EventType::Killjoy,				5 } },
	{ L"Avenger!",													{ EventType::Avenger,				5 } },
	{ L"#effect_player avenged your death.",						{ EventType::Avenger,				5 } },
	{ L"Pull!",														{ EventType::Pull,					5 } },
	{ L"#cause_player struck #effect_player down!",					{ EventType::Struck,				5 } },
	{ L"#cause_player struck you down!",							{ EventType::Struck,				5 } },
	{ L"#cause_player killed #effect_player with grenades",			{ EventType::KillWithGrenades,		5 } },
	{ L"#effect_player was killed by #cause_player with grenades",	{ EventType::KillWithGrenades,		5 } },
	{ L"You killed #effect_player with grenades",					{ EventType::KillWithGrenades,		5 } },
	{ L"#cause_player killed you with grenades",					{ EventType::KillWithGrenades,		5 } },
	{ L"#cause_player lasered #effect_player",						{ EventType::Lasered,				5 } },
	{ L"#effect_player was lasered by #cause_player",				{ EventType::Lasered,				5 } },
	{ L"You lasered #effect_player",								{ EventType::Lasered,				5 } },
	{ L"#cause_player lasered you",									{ EventType::Lasered,				5 } },
	{ L"#cause_player sniped #effect_player",						{ EventType::Sniped,				5 } },
	{ L"#cause_player sniped you",									{ EventType::Sniped,				5 } },
	{ L"You sniped #effect_player",									{ EventType::Sniped,				5 } },
	{ L"Assist",													{ EventType::Assist,				0 } },
	{ L"#cause_player killed #effect_player",						{ EventType::Kill,					0 } },
	{ L"You killed #effect_player",									{ EventType::Kill,					0 } },
	{ L"#cause_player killed you",									{ EventType::Kill,					0 } },
	{ L"#cause_player committed suicide",							{ EventType::Suicide,				0 } },
	{ L"You committed suicide",										{ EventType::Suicide,				0 } },
	{ L"#cause_player betrayed #effect_player",						{ EventType::Betrayal,				0 } },
	{ L"You betrayed #effect_player",								{ EventType::Betrayal,				0 } },
	{ L"#cause_player betrayed you",								{ EventType::Betrayal,				0 } },
	{ L"Killed by the Guardians",									{ EventType::KilledByTheGuardians,	0 } },
	{ L"You fell to your death",									{ EventType::FellToYourDeath,		0 } },
};

// 00 - 07
struct Teams 
{
	int8_t First;
	int8_t Second;
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