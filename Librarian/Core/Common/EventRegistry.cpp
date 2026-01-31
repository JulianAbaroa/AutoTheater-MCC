#include "pch.h"
#include "Core/Common/EventRegistry.h"

static EventClass ResolveEventClass(EventType type)
{
	switch (type)
	{
	case EventType::Join:
	case EventType::Rejoin:
	case EventType::Quit:
	case EventType::Booted:
	case EventType::JoinedTeam:
		return EventClass::Server;

	case EventType::TookLead:
	case EventType::LostLead:
	case EventType::TiedLead:
	case EventType::TeamScored:
	case EventType::OneMinuteToWin:
	case EventType::ThirtySecondsToWin:
	case EventType::OneMinuteRemaining:
	case EventType::ThirtySecondsRemaining:
	case EventType::TenSecondsRemaining:
	case EventType::RoundOver:
	case EventType::GameOver:
		return EventClass::Match;

	case EventType::Custom:
		return EventClass::Custom;

	case EventType::CaptureTheFlag:
	case EventType::FlagCaptured:
	case EventType::FlagStolen:
	case EventType::FlagDropped:
	case EventType::FlagReseted:
	case EventType::FlagRecovered:
		return EventClass::CaptureTheFlag;

	case EventType::Slayer:
		return EventClass::Slayer;

	case EventType::Race:
		return EventClass::Race;

	case EventType::Assault:
	case EventType::HasTheBomb:
	case EventType::ArmedTheBomb:
	case EventType::DroppedTheBomb:
	case EventType::BombReset:
		return EventClass::Assault;

	case EventType::Juggernaut:
	case EventType::IsTheJuggernaut:
	case EventType::YouKilledTheJuggernaut:
		return EventClass::Juggernaut;

	case EventType::KingOfTheHill:
	case EventType::ControlsTheHill:
	case EventType::HillContested:
	case EventType::HillMoved:
		return EventClass::KingOfTheHill;

	case EventType::Territories:
	case EventType::TeamCapturedATerritory:
	case EventType::TerritoryContested:
	case EventType::TerritoryLost:
		return EventClass::Territories;

	case EventType::Infection:
	case EventType::YouAreAZombie:
	case EventType::Infected:
	case EventType::IsTheLastManStanding:
	case EventType::ZombiesWin:
	case EventType::SurvivorsWin:
		return EventClass::Infection;

	case EventType::Oddball:
	case EventType::PlayBall:
	case EventType::PickedUpTheBall:
	case EventType::BallDropped:
	case EventType::BallReset:
		return EventClass::Oddball;

	case EventType::Killionaire:
	case EventType::Killpocalypse:
	case EventType::Killtastrophe:
	case EventType::Killimanjaro:
	case EventType::Killtrocity:
	case EventType::Killtacular:
	case EventType::Stopped:
	case EventType::Overkill:
	case EventType::Stuck:
	case EventType::Assassinated:
	case EventType::ShowStopper:
	case EventType::Yoink:
	case EventType::TripleKill:
	case EventType::DoubleKill:
	case EventType::BeatDown:
	case EventType::Headcase:
	case EventType::Bulltrue:
	case EventType::SplatterSpree:
	case EventType::VehicularManslaughter:
	case EventType::SundayDriver:
	case EventType::StickySpree:
	case EventType::StickyFingers:
	case EventType::Corrected:
	case EventType::ShotgunSpree:
	case EventType::OpenSeason:
	case EventType::BuckWild:
	case EventType::WreckingCrew:
	case EventType::Dreamcrusher:
	case EventType::HammerSpree:
	case EventType::SwordSpree:
	case EventType::SliceNDice:
	case EventType::CuttingCrew:
	case EventType::JuggernautSpree:
	case EventType::Wheelman:
	case EventType::WheelmanSpree:
	case EventType::Roadhog:
	case EventType::Roadrage:
	case EventType::KillingSpree:
	case EventType::KillingFrenzy:
	case EventType::RunningRiot:
	case EventType::Rampage:
	case EventType::Untouchable:
	case EventType::Invincible:
	case EventType::Inconceivable:
	case EventType::Unfrigginbelievable:
	case EventType::Splattered:
	case EventType::Pummeled:
	case EventType::Headshot:
	case EventType::KillFromTheGrave:
	case EventType::Revenge:
	case EventType::FirstStrike:
	case EventType::ReloadThis:
	case EventType::CloseCall:
	case EventType::Protector:
	case EventType::Firebird:
	case EventType::Killjoy:
	case EventType::Avenger:
	case EventType::Pull:
	case EventType::Struck:
	case EventType::KillWithGrenades:
	case EventType::Lasered:
	case EventType::Sniped:
	case EventType::SecondGunman:
	case EventType::Sidekick:
	case EventType::AssistSpree:
	case EventType::Assist:
	case EventType::Kill:
	case EventType::Suicide:
	case EventType::Betrayal:
	case EventType::KilledByTheGuardians:
	case EventType::FellToYourDeath:
		return EventClass::KillRelated;

	default: 
		return EventClass::Unknown;
	}
}

static EventInfo CreateEvent(EventType type, int weight)
{
	return { type, weight, ResolveEventClass(type) };
}

// TODO: Add all of them
std::unordered_map<std::wstring, EventInfo> g_EventRegistry = {
	// Server
	{ L"#cause_player joined the game",								CreateEvent(EventType::Join,		0) },
	{ L"#cause_player rejoined the game",							CreateEvent(EventType::Rejoin,		0) },
	{ L"#cause_player quit",										CreateEvent(EventType::Quit,		0) },
	{ L"#effect_player was booted",									CreateEvent(EventType::Booted,		0) },
	{ L"#cause_player joined the #effect_team",						CreateEvent(EventType::JoinedTeam,	0) },
	{ L"#cause_player joined your team",							CreateEvent(EventType::JoinedTeam,	0) },
	{ L"You joined the #effect_team",								CreateEvent(EventType::JoinedTeam,	0) },

	// Match
	{ L"#cause_team took the lead!",								CreateEvent(EventType::TookLead,				0) },
	{ L"#cause_player took the lead!",								CreateEvent(EventType::TookLead,				0) },
	{ L"Your team took the lead!",									CreateEvent(EventType::TookLead,				0) },
	{ L"You took the lead!",										CreateEvent(EventType::TookLead,				0) },
	{ L"You lost the lead",											CreateEvent(EventType::LostLead,				0) },
	{ L"Your team lost the lead",									CreateEvent(EventType::LostLead,				0) },
	{ L"#cause_team tied for the lead!",							CreateEvent(EventType::TiedLead,				0) },
	{ L"You are tied for the lead!",								CreateEvent(EventType::TiedLead,				0) },
	{ L"Your team is tied for the lead!",							CreateEvent(EventType::TiedLead,				0) },
	{ L"#cause_player tied for the lead!",							CreateEvent(EventType::TiedLead,				0) },
	{ L"#cause_team scored", 										CreateEvent(EventType::TeamScored,				0) },
	{ L"1 minute to win!",											CreateEvent(EventType::OneMinuteToWin,			75) },
	{ L"#cause_player 1 minute to win!",							CreateEvent(EventType::OneMinuteToWin,			75) },
	{ L"30 seconds to win!",										CreateEvent(EventType::ThirtySecondsToWin,		75) },
	{ L"#cause_player 30 seconds to win!",							CreateEvent(EventType::ThirtySecondsToWin,		75) },
	{ L"1 minute remaining",										CreateEvent(EventType::OneMinuteRemaining,		0) },
	{ L"30 seconds remaining",										CreateEvent(EventType::ThirtySecondsRemaining,	0) },
	{ L"10 seconds remaining",										CreateEvent(EventType::TenSecondsRemaining,		0) },
	{ L"Round over",												CreateEvent(EventType::RoundOver,				0) },
	{ L"Game over",													CreateEvent(EventType::GameOver,				0) },

	// Custom
	{ L"Custom",													CreateEvent(EventType::Custom,					0) },

	// CTF
	{ L"Capture the Flag",											CreateEvent(EventType::CaptureTheFlag,			0) },
	{ L"#cause_team captured a flag!",								CreateEvent(EventType::FlagCaptured,			100) },
	{ L"You captured a flag!",										CreateEvent(EventType::FlagCaptured,			100) },
	{ L"#effect_team flag was stolen!",								CreateEvent(EventType::FlagStolen,				75) },
	{ L"Your flag has been stolen!",								CreateEvent(EventType::FlagStolen,				75) }, // check this two
	{ L"You grabbed the #effect_team flag!",						CreateEvent(EventType::FlagStolen,				75) }, // check this two
	{ L"#effect_team flag dropped",									CreateEvent(EventType::FlagDropped,				0) },
	{ L"You dropped #effect_team flag",								CreateEvent(EventType::FlagDropped,				0) },
	{ L"#effect_team flag reset",									CreateEvent(EventType::FlagReseted,				0) },
	{ L"#effect_team flag recovered",								CreateEvent(EventType::FlagRecovered,			0) },
	{ L"Your flag has been recovered",								CreateEvent(EventType::FlagRecovered,			0) },

	// Slayer
	{ L"Slayer",													CreateEvent(EventType::Slayer,					0) },

	// Race 
	{ L"Race",														CreateEvent(EventType::Race,					0) },

	// Assault
	{ L"Assault",													CreateEvent(EventType::Assault,					0) },
	{ L"You picked up the bomb", 									CreateEvent(EventType::HasTheBomb,				75) },
	{ L"You armed the bomb",										CreateEvent(EventType::ArmedTheBomb,			100) },
	{ L"#cause_player armed the bomb",								CreateEvent(EventType::ArmedTheBomb,			100) },
	{ L"You dropped the bomb",										CreateEvent(EventType::DroppedTheBomb,			0) },
	{ L"Bomb reset",												CreateEvent(EventType::BombReset,				0) },

	// Juggernaut
	{ L"Juggernaut",												CreateEvent(EventType::Juggernaut,				0) },
	{ L"You are the juggernaut",									CreateEvent(EventType::IsTheJuggernaut,			75) },
	{ L"#cause_player is the juggernaut",							CreateEvent(EventType::IsTheJuggernaut,			75) },
	{ L"You killed the juggernaut",									CreateEvent(EventType::YouKilledTheJuggernaut,	75) },

	// KOTH
	{ L"King of the Hill",											CreateEvent(EventType::KingOfTheHill,			0) },
	{ L"#cause_player controls the hill", 							CreateEvent(EventType::ControlsTheHill,			75) },
	{ L"You control the hill", 										CreateEvent(EventType::ControlsTheHill,			75) },
	{ L"Your team controls the hill", 								CreateEvent(EventType::ControlsTheHill,			75) },
	{ L"Hill contested",											CreateEvent(EventType::HillContested,			0) },
	{ L"Hill moved",												CreateEvent(EventType::HillMoved,				0) },

	// Territories
	{ L"Territories",												CreateEvent(EventType::Territories,				0) },
	{ L"#cause_team captured a territory",							CreateEvent(EventType::TeamCapturedATerritory,	0) },
	{ L"Your team captured a territory",							CreateEvent(EventType::TeamCapturedATerritory,	0) },
	{ L"Territory contested",										CreateEvent(EventType::TerritoryContested,		0) },
	{ L"Territory lost",											CreateEvent(EventType::TerritoryLost,			0) },

	// Infection
	{ L"Infection",													CreateEvent(EventType::Infection,				0) },
	{ L"You are a zombie",											CreateEvent(EventType::YouAreAZombie,			0) },
	{ L"You infected #effect_player",								CreateEvent(EventType::Infected,				75) },
	{ L"#cause_player infected you",								CreateEvent(EventType::Infected,				75) },
	{ L"#cause_player is the last man standing",					CreateEvent(EventType::IsTheLastManStanding,	0) },
	{ L"You are the last man standing",								CreateEvent(EventType::IsTheLastManStanding,	0) },
	{ L"Zombies Win!",												CreateEvent(EventType::ZombiesWin,				0) },
	{ L"Survivors Win!",											CreateEvent(EventType::SurvivorsWin,			0) },

	// Oddball
	{ L"Oddball",													CreateEvent(EventType::Oddball,					0) },
	{ L"Play ball!",												CreateEvent(EventType::PlayBall,				0) },
	{ L"#cause_player picked up the ball",							CreateEvent(EventType::PickedUpTheBall,			75) },
	{ L"You picked up the ball",									CreateEvent(EventType::PickedUpTheBall,			75) },
	{ L"Ball dropped",												CreateEvent(EventType::BallDropped,				0) },
	{ L"You dropped the ball",										CreateEvent(EventType::BallDropped,				0) },
	{ L"Ball reset",												CreateEvent(EventType::BallReset,				0) },

	// Kill
	{ L"Killionaire!",												CreateEvent(EventType::Killionaire,				100) },
	{ L"Killpocalypse!",											CreateEvent(EventType::Killpocalypse,			90) },
	{ L"Killtastrophe!",											CreateEvent(EventType::Killtastrophe,			80) },
	{ L"Killimanjaro!",												CreateEvent(EventType::Killimanjaro,			70) },
	{ L"Killtrocity!",												CreateEvent(EventType::Killtrocity,				60) },
	{ L"Killtacular!",												CreateEvent(EventType::Killtacular,				50) },
	{ L"#cause_player stopped #effect_player",						CreateEvent(EventType::Stopped,					50) },
	{ L"You stopped #effect_player",								CreateEvent(EventType::Stopped,					50) }, // check this two
	{ L"#cause_player stopped you",									CreateEvent(EventType::Stopped,					50) }, // check this two
	{ L"Overkill!",													CreateEvent(EventType::Overkill,				40) },
	{ L"#cause_player stuck #effect_player",						CreateEvent(EventType::Stuck,					30) },
	{ L"#cause_player stuck you" ,									CreateEvent(EventType::Stuck,					30) },
	{ L"You stuck #effect_player",									CreateEvent(EventType::Stuck,					30) },
	{ L"#cause_player assassinated #effect_player",					CreateEvent(EventType::Assassinated,			30) },
	{ L"#effect_player was assassinated by #cause_player",			CreateEvent(EventType::Assassinated,			30) },
	{ L"#cause_player assassinated you",							CreateEvent(EventType::Assassinated,			30) },
	{ L"You assassinated #effect_player",							CreateEvent(EventType::Assassinated,			30) },
	{ L"You were killed while performing an assassination.",		CreateEvent(EventType::ShowStopper,				30) },
	{ L"Showstopper!",												CreateEvent(EventType::ShowStopper,				30) },
	{ L"Yoink!",													CreateEvent(EventType::Yoink,					30) },
	{ L"Triple Kill!",												CreateEvent(EventType::TripleKill,				30) },
	{ L"Double Kill!",												CreateEvent(EventType::DoubleKill,				20) },
	{ L"#cause_player beat down #effect_player",					CreateEvent(EventType::BeatDown,				20) },
	{ L"#cause_player beat you down",								CreateEvent(EventType::BeatDown,				20) },
	{ L"You beat down #effect_player",								CreateEvent(EventType::BeatDown,				20) },
	{ L"Headcase!",													CreateEvent(EventType::Headcase,				20) },
	{ L"Bulltrue!",													CreateEvent(EventType::Bulltrue,				20) },
	{ L"Splatter Spree!",											CreateEvent(EventType::SplatterSpree,			10) },
	{ L"Vehicular Manslaughter!",									CreateEvent(EventType::VehicularManslaughter,	10) },
	{ L"Sunday Driver!",											CreateEvent(EventType::SundayDriver,			10) },
	{ L"Sticky Spree!",												CreateEvent(EventType::StickySpree,				10) },
	{ L"Sticky Fingers!",											CreateEvent(EventType::StickyFingers,			10) },
	{ L"Corrected!",												CreateEvent(EventType::Corrected,				10) },
	{ L"Shotgun Spree!",											CreateEvent(EventType::ShotgunSpree,			10) },
	{ L"Open Season!",												CreateEvent(EventType::OpenSeason,				10) },
	{ L"Buck Wild!",												CreateEvent(EventType::BuckWild,				10) },
	{ L"Wrecking Crew!",											CreateEvent(EventType::WreckingCrew,			10) },
	{ L"Dreamcrusher!",												CreateEvent(EventType::Dreamcrusher,			10) },
	{ L"Hammer Spree!",												CreateEvent(EventType::HammerSpree,				10) },
	{ L"Sword Spree!",												CreateEvent(EventType::SwordSpree,				10) },
	{ L"Slice 'n Dice!",											CreateEvent(EventType::SliceNDice,				10) },
	{ L"Cutting Crew!", 											CreateEvent(EventType::CuttingCrew,				10) },

	// TODO: Get the other ones
	{ L"Juggernaut Spree!",											CreateEvent(EventType::JuggernautSpree,			10) },

	{ L"#cause_player's driving assisted you",						CreateEvent(EventType::Wheelman,				10) },
	{ L"Your driving assisted #effect_player",						CreateEvent(EventType::Wheelman,				10) },
	{ L"Wheelman Spree!",											CreateEvent(EventType::WheelmanSpree,			10) },
	{ L"Road Hog!",													CreateEvent(EventType::Roadhog,					10) },
	{ L"Road Rage!",												CreateEvent(EventType::Roadrage,				10) },
	{ L"#cause_player is on a Killing Spree!",						CreateEvent(EventType::KillingSpree,			10) },
	{ L"Killing Spree!",											CreateEvent(EventType::KillingSpree,			10) },
	{ L"#cause_player is on a Killing Frenzy!",						CreateEvent(EventType::KillingFrenzy,			10) },
	{ L"Killing Frenzy!",											CreateEvent(EventType::KillingFrenzy,			10) },
	{ L"#cause_player is a Running Riot!",							CreateEvent(EventType::RunningRiot,				10) },
	{ L"Running Riot!",												CreateEvent(EventType::RunningRiot,				10) },
	{ L"#cause_player is on a Rampage!",							CreateEvent(EventType::Rampage,					10) },
	{ L"Rampage!",													CreateEvent(EventType::Rampage,					10) },
	{ L"#cause_player is Untouchable!",								CreateEvent(EventType::Untouchable,				10) },
	{ L"Untouchable!",												CreateEvent(EventType::Untouchable,				10) },
	{ L"#cause_player is Invincible!",								CreateEvent(EventType::Invincible,				10) },
	{ L"Invincible!",												CreateEvent(EventType::Invincible,				10) },
	{ L"#cause_player is Inconceivable!",							CreateEvent(EventType::Inconceivable,			10) },
	{ L"Inconceivable!",											CreateEvent(EventType::Inconceivable,			10) },
	{ L"#cause_player is Unfrigginbelievable!",						CreateEvent(EventType::Unfrigginbelievable,		10) },
	{ L"Unfrigginbelievable!",										CreateEvent(EventType::Unfrigginbelievable,		10) },
	{ L"#cause_player splattered #effect_player",					CreateEvent(EventType::Splattered,				10) },
	{ L"You splattered #effect_player",								CreateEvent(EventType::Splattered,				10) },
	{ L"#cause_player splattered you",								CreateEvent(EventType::Splattered,				10) },
	{ L"#cause_player pummeled #effect_player",						CreateEvent(EventType::Pummeled,				10) },
	{ L"#cause_player pummeled you",								CreateEvent(EventType::Pummeled,				10) },
	{ L"You pummeled #effect_player",								CreateEvent(EventType::Pummeled,				10) },
	{ L"#cause_player killed #effect_player with a headshot",		CreateEvent(EventType::Headshot,				10) },
	{ L"#effect_player was killed by a #cause_player headshot",		CreateEvent(EventType::Headshot,				10) },
	{ L"#cause_player killed you with a headshot",					CreateEvent(EventType::Headshot,				10) },
	{ L"You killed #effect_player with a headshot",					CreateEvent(EventType::Headshot,				10) },
	{ L"#cause_player killed you from the grave",					CreateEvent(EventType::KillFromTheGrave,		5) },
	{ L"You killed #effect_player from the grave",					CreateEvent(EventType::KillFromTheGrave,		5) },
	{ L"#cause_player got revenge!",								CreateEvent(EventType::Revenge,					5) },
	{ L"Revenge!",													CreateEvent(EventType::Revenge,					5) },
	{ L"First Strike!",												CreateEvent(EventType::FirstStrike,				5) },
	{ L"Reload This!",												CreateEvent(EventType::ReloadThis,				5) },
	{ L"You were killed while reloading",							CreateEvent(EventType::ReloadThis,				5) },
	{ L"Close Call!",												CreateEvent(EventType::CloseCall,				5) },
	{ L"Protector!",												CreateEvent(EventType::Protector,				5) },
	{ L"#effect_player saved your life.",							CreateEvent(EventType::Protector,				5) },
	{ L"Firebird!",													CreateEvent(EventType::Firebird,				5) },
	{ L"Killjoy!",													CreateEvent(EventType::Killjoy,					5) },
	{ L"#cause_player ended your spree",							CreateEvent(EventType::Killjoy,					5) },
	{ L"Avenger!",													CreateEvent(EventType::Avenger,					5) },
	{ L"#effect_player avenged your death.",						CreateEvent(EventType::Avenger,					5) },
	{ L"Pull!",														CreateEvent(EventType::Pull,					5) },
	{ L"#cause_player struck #effect_player down!",					CreateEvent(EventType::Struck,					5) },
	{ L"#cause_player struck you down!",							CreateEvent(EventType::Struck,					5) },
	{ L"#cause_player killed #effect_player with grenades",			CreateEvent(EventType::KillWithGrenades,		5) },
	{ L"#effect_player was killed by #cause_player with grenades",	CreateEvent(EventType::KillWithGrenades,		5) },
	{ L"You killed #effect_player with grenades",					CreateEvent(EventType::KillWithGrenades,		5) },
	{ L"#cause_player killed you with grenades",					CreateEvent(EventType::KillWithGrenades,		5) },
	{ L"#cause_player lasered #effect_player",						CreateEvent(EventType::Lasered,					5) },
	{ L"#effect_player was lasered by #cause_player",				CreateEvent(EventType::Lasered,					5) },
	{ L"You lasered #effect_player",								CreateEvent(EventType::Lasered,					5) },
	{ L"#cause_player lasered you",									CreateEvent(EventType::Lasered,					5) },
	{ L"#cause_player sniped #effect_player",						CreateEvent(EventType::Sniped,					5) },
	{ L"#cause_player sniped you",									CreateEvent(EventType::Sniped,					5) },
	{ L"You sniped #effect_player",									CreateEvent(EventType::Sniped,					5) },
	{ L"Second Gunman!",											CreateEvent(EventType::SecondGunman,			5) },
	{ L"Sidekick!",													CreateEvent(EventType::Sidekick,				5) },
	{ L"Assist Spree!",												CreateEvent(EventType::AssistSpree,				5) },
	{ L"Assist",													CreateEvent(EventType::Assist,					0) },
	{ L"#cause_player killed #effect_player",						CreateEvent(EventType::Kill,					0) },
	{ L"You killed #effect_player",									CreateEvent(EventType::Kill,					0) },
	{ L"#cause_player killed you",									CreateEvent(EventType::Kill,					0) },
	{ L"#cause_player committed suicide",							CreateEvent(EventType::Suicide,					0) },
	{ L"You committed suicide",										CreateEvent(EventType::Suicide,					0) },
	{ L"#cause_player betrayed #effect_player",						CreateEvent(EventType::Betrayal,				0) },
	{ L"You betrayed #effect_player",								CreateEvent(EventType::Betrayal,				0) },
	{ L"#cause_player betrayed you",								CreateEvent(EventType::Betrayal,				0) },
	{ L"Killed by the Guardians",									CreateEvent(EventType::KilledByTheGuardians,	0) },
	{ L"You fell to your death",									CreateEvent(EventType::FellToYourDeath,			0) },
};