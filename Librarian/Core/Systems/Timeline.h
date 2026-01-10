#pragma once

#include "Core/Systems/Theater.h"
#include <unordered_set>
#include <vector>
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
	Join, Rejoin, Quit, Booted, Banned, MinutesRemaining,

	// Gametypes
	CaptureTheFlag,

	// Match
	TookLead, TiedLead, GameOver, Wins,

	// Death types
	Kill, Betrayal, Suicide,

	// Objective types
	ObjectiveStolen, ObjectiveCaptured, ObjectiveReseted,
	ObjectiveRecovered, ObjectiveDropped,

	// Kill types
	Headshot, Pummeled, Stuck, Stopped,
	KillingSpree, Splattered, BeatDown, Assassinated,
	KillingFrenzy, RunningRiot, Lasered, Sniped, Rampage,
	// These doens't have a player name attached to it
	Struck, DoubleKill, TripleKill, SwordSpree, ReloadThis,
	Revenge, Killjoy, Avenger, Pull, Headcase, Assist, CloseCall,
	FirstStrike, Firebird, Protector, Overkill, HammerSpree

	// Keep adding game events...
};

// TODO: SEE IF ALL THE EVENTS ARE CORRECTLY PARSED
const std::map<EventType, EventInfo> g_EventRegistry = {
	// Server
	{ EventType::Join,				{ L"joined",			0 } },
	{ EventType::Rejoin,			{ L"rejoined",			0 } },
	{ EventType::Quit,				{ L"quit",				0 } },
	{ EventType::Booted,			{ L"booted",			0 } },
	{ EventType::Banned,			{ L"banned",			0 } },
	{ EventType::MinutesRemaining,	{ L"minutesremaining",	0 } },

	// Gametype
	{ EventType::CaptureTheFlag,	{ L"capturetheflag",	0 } },

	// Match
	{ EventType::TookLead,			{ L"tookthelead",		0 } },
	{ EventType::TiedLead,			{ L"tiedforthelead",	0 } },
	{ EventType::GameOver,			{ L"gameover",			0 } },
	{ EventType::Wins,				{ L"wins",				0 } },

	// Objective
	{ EventType::ObjectiveCaptured,	{ L"youcapturedaflag",	100 } },
	{ EventType::ObjectiveStolen,	{ L"yougrabbedthe",		75 } },
	{ EventType::ObjectiveReseted,	{ L"flagreset",			0 } },
	{ EventType::ObjectiveRecovered,{ L"flagrecovered",		0 } },
	{ EventType::ObjectiveDropped,	{ L"flagdropped",		0 } },

	// Kill
	{ EventType::Stopped,			{ L"stopped",			50 } },
	{ EventType::Overkill,			{ L"overkill",			40 } },
	{ EventType::Stuck,				{ L"stuck",				30 } },
	{ EventType::Assassinated,		{ L"assassinated",		30 } },
	{ EventType::TripleKill,		{ L"triplekill",		30 } },
	{ EventType::BeatDown,			{ L"beatdown",			20 } },
	{ EventType::Headcase,			{ L"headcase",			20 } },
	{ EventType::DoubleKill,		{ L"doublekill",		20 } },
	{ EventType::Splattered,		{ L"splattered",		10 } },
	{ EventType::Headshot,			{ L"headshot",			10 } },
	{ EventType::Pummeled,			{ L"pummeled",			10 } },
	{ EventType::KillingSpree,		{ L"killingspree",		10 } },
	{ EventType::SwordSpree,		{ L"swordspree",		10 } },
	{ EventType::HammerSpree,		{ L"hammerspree",		10 } },
	{ EventType::KillingFrenzy,		{ L"killingfrenzy",		10 } },
	{ EventType::RunningRiot,		{ L"runningriot",		10 } },
	{ EventType::Rampage,			{ L"rampage",			10 } },
	{ EventType::Struck,			{ L"struck",			5  } },
	{ EventType::ReloadThis,		{ L"reloadthis",		5 } },
	{ EventType::Revenge,			{ L"revenge",			5 } },
	{ EventType::Killjoy,			{ L"killjoy",			5 } },
	{ EventType::Avenger,			{ L"avenger",			5 } },
	{ EventType::Pull,				{ L"pull",				5 } },
	{ EventType::CloseCall,			{ L"closecall",			5 } },
	{ EventType::FirstStrike,		{ L"firststrike",		5 } },
	{ EventType::Firebird,			{ L"firebird",			5 } },
	{ EventType::Protector,			{ L"protector",			5 } },
	{ EventType::Lasered,			{ L"lasered",			5 } },
	{ EventType::Sniped,			{ L"sniped",			5 } },
	{ EventType::Assist,			{ L"assist",			0 } },
	{ EventType::Kill,				{ L"killed",			0 } },
	{ EventType::Betrayal,			{ L"betrayed",			0 } },
	{ EventType::Suicide,			{ L"suicide",			0 } },
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