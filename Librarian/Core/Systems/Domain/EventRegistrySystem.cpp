#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include <fstream>

EventRegistrySystem::EventRegistrySystem()
{
	this->InitializeDefaultRegistry();
}

void EventRegistrySystem::SaveEventRegistry()
{
	std::string path = g_pState->Settings.GetAppDataDirectory() + "\\event_weights.cfg";;
	auto eventRegistryCopy = g_pState->EventRegistry.GetEventRegistryCopy();

	std::ofstream file(path);
	if (!file.is_open()) return;

	file << "# AutoTheater Event Weights\n";
	for (const auto& [name, info] : eventRegistryCopy)
	{
		file << g_pUtil->Format.WStringToString(name) << "=" << info.Weight << "\n";
	}

	file.close();
}

void EventRegistrySystem::LoadEventRegistry()
{
	std::string path = g_pState->Settings.GetAppDataDirectory() + "\\event_weights.cfg";
	auto eventRegistryCopy = g_pState->EventRegistry.GetEventRegistryCopy();;

	std::ifstream file(path);
	if (!file.is_open()) return;

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#') continue;

		size_t delimiterPos = line.find_last_of('=');
		if (delimiterPos != std::string::npos)
		{
			std::string nameStr = line.substr(0, delimiterPos);
			std::string weightStr = line.substr(delimiterPos + 1);

			std::wstring eventName = g_pUtil->Format.StringToWString(nameStr);

			if (eventRegistryCopy.count(eventName))
			{
				try {
					eventRegistryCopy[eventName].Weight = std::stoi(weightStr);
				}
				catch (...) { }
			}
		}
	}

	file.close();

	g_pState->EventRegistry.SetEventRegistry(eventRegistryCopy);
}


void EventRegistrySystem::InitializeDefaultRegistry()
{
	std::unordered_map<std::wstring, EventInfo> tempMap;

	// TODO: Add all of them
	tempMap = {
		// Server
		{ L"#cause_player joined the game",								this->BuildEvent(EventType::Join,					0) },
		{ L"#cause_player rejoined the game",							this->BuildEvent(EventType::Rejoin,					0) },
		{ L"#cause_player quit",										this->BuildEvent(EventType::Quit,					0) },
		{ L"#effect_player was booted",									this->BuildEvent(EventType::Booted,					0) },
		{ L"#cause_player joined the #effect_team",						this->BuildEvent(EventType::JoinedTeam,				0) },
		{ L"#cause_player joined your team",							this->BuildEvent(EventType::JoinedTeam,				0) },
		{ L"You joined the #effect_team",								this->BuildEvent(EventType::JoinedTeam,				0) },

		// Match
		{ L"#cause_team took the lead!",								this->BuildEvent(EventType::TookLead,				0) },
		{ L"#cause_player took the lead!",								this->BuildEvent(EventType::TookLead,				0) },
		{ L"Your team took the lead!",									this->BuildEvent(EventType::TookLead,				0) },
		{ L"You took the lead!",										this->BuildEvent(EventType::TookLead,				0) },
		{ L"You lost the lead",											this->BuildEvent(EventType::LostLead,				0) },
		{ L"Your team lost the lead",									this->BuildEvent(EventType::LostLead,				0) },
		{ L"#cause_team tied for the lead!",							this->BuildEvent(EventType::TiedLead,				0) },
		{ L"You are tied for the lead!",								this->BuildEvent(EventType::TiedLead,				0) },
		{ L"Your team is tied for the lead!",							this->BuildEvent(EventType::TiedLead,				0) },
		{ L"#cause_player tied for the lead!",							this->BuildEvent(EventType::TiedLead,				0) },
		{ L"#cause_team scored", 										this->BuildEvent(EventType::TeamScored,				0) },
		{ L"1 minute to win!",											this->BuildEvent(EventType::OneMinuteToWin,			75) },
		{ L"#cause_player 1 minute to win!",							this->BuildEvent(EventType::OneMinuteToWin,			75) },
		{ L"30 seconds to win!",										this->BuildEvent(EventType::ThirtySecondsToWin,		75) },
		{ L"#cause_player 30 seconds to win!",							this->BuildEvent(EventType::ThirtySecondsToWin,		75) },
		{ L"1 minute remaining",										this->BuildEvent(EventType::OneMinuteRemaining,		0) },
		{ L"30 seconds remaining",										this->BuildEvent(EventType::ThirtySecondsRemaining,	0) },
		{ L"10 seconds remaining",										this->BuildEvent(EventType::TenSecondsRemaining,	0) },
		{ L"Round over",												this->BuildEvent(EventType::RoundOver,				0) },
		{ L"Game over",													this->BuildEvent(EventType::GameOver,				0) },

		// Custom
		{ L"Custom",													this->BuildEvent(EventType::Custom,					0) },

		// CTF
		{ L"Capture the Flag",											this->BuildEvent(EventType::CaptureTheFlag,			0) },
		{ L"#cause_team captured a flag!",								this->BuildEvent(EventType::FlagCaptured,			100) },
		{ L"You captured a flag!",										this->BuildEvent(EventType::FlagCaptured,			100) },
		{ L"#effect_team flag was stolen!",								this->BuildEvent(EventType::FlagStolen,				75) },
		{ L"Your flag has been stolen!",								this->BuildEvent(EventType::FlagStolen,				75) }, // check this two
		{ L"You grabbed the #effect_team flag!",						this->BuildEvent(EventType::FlagStolen,				75) }, // check this two
		{ L"#effect_team flag dropped",									this->BuildEvent(EventType::FlagDropped,			0) },
		{ L"You dropped #effect_team flag",								this->BuildEvent(EventType::FlagDropped,			0) },
		{ L"#effect_team flag reset",									this->BuildEvent(EventType::FlagReseted,			0) },
		{ L"#effect_team flag recovered",								this->BuildEvent(EventType::FlagRecovered,			0) },
		{ L"Your flag has been recovered",								this->BuildEvent(EventType::FlagRecovered,			0) },

		// Slayer
		{ L"Slayer",													this->BuildEvent(EventType::Slayer,					0) },

		// Race 
		{ L"Race",														this->BuildEvent(EventType::Race,					0) },

		// Assault
		{ L"Assault",													this->BuildEvent(EventType::Assault,				0) },
		{ L"You picked up the bomb", 									this->BuildEvent(EventType::HasTheBomb,				75) },
		{ L"You armed the bomb",										this->BuildEvent(EventType::ArmedTheBomb,			100) },
		{ L"#cause_player armed the bomb",								this->BuildEvent(EventType::ArmedTheBomb,			100) },
		{ L"You dropped the bomb",										this->BuildEvent(EventType::DroppedTheBomb,			0) },
		{ L"Bomb reset",												this->BuildEvent(EventType::BombReset,				0) },

		// Juggernaut
		{ L"Juggernaut",												this->BuildEvent(EventType::Juggernaut,				0) },
		{ L"You are the juggernaut",									this->BuildEvent(EventType::IsTheJuggernaut,		75) },
		{ L"#cause_player is the juggernaut",							this->BuildEvent(EventType::IsTheJuggernaut,		75) },
		{ L"You killed the juggernaut",									this->BuildEvent(EventType::YouKilledTheJuggernaut,	75) },

		// KOTH
		{ L"King of the Hill",											this->BuildEvent(EventType::KingOfTheHill,			0) },
		{ L"#cause_player controls the hill", 							this->BuildEvent(EventType::ControlsTheHill,		75) },
		{ L"You control the hill", 										this->BuildEvent(EventType::ControlsTheHill,		75) },
		{ L"Your team controls the hill", 								this->BuildEvent(EventType::ControlsTheHill,		75) },
		{ L"Hill contested",											this->BuildEvent(EventType::HillContested,			0) },
		{ L"Hill moved",												this->BuildEvent(EventType::HillMoved,				0) },

		// Territories
		{ L"Territories",												this->BuildEvent(EventType::Territories,			0) },
		{ L"#cause_team captured a territory",							this->BuildEvent(EventType::TeamCapturedATerritory,	0) },
		{ L"Your team captured a territory",							this->BuildEvent(EventType::TeamCapturedATerritory,	0) },
		{ L"Territory contested",										this->BuildEvent(EventType::TerritoryContested,		0) },
		{ L"Territory lost",											this->BuildEvent(EventType::TerritoryLost,			0) },

		// Infection
		{ L"Infection",													this->BuildEvent(EventType::Infection,				0) },
		{ L"You are a zombie",											this->BuildEvent(EventType::YouAreAZombie,			0) },
		{ L"You infected #effect_player",								this->BuildEvent(EventType::Infected,				75) },
		{ L"#cause_player infected you",								this->BuildEvent(EventType::Infected,				75) },
		{ L"#cause_player is the last man standing",					this->BuildEvent(EventType::IsTheLastManStanding,	0) },
		{ L"You are the last man standing",								this->BuildEvent(EventType::IsTheLastManStanding,	0) },
		{ L"Zombies Win!",												this->BuildEvent(EventType::ZombiesWin,				0) },
		{ L"Survivors Win!",											this->BuildEvent(EventType::SurvivorsWin,			0) },

		// Oddball
		{ L"Oddball",													this->BuildEvent(EventType::Oddball,				0) },
		{ L"Play ball!",												this->BuildEvent(EventType::PlayBall,				0) },
		{ L"#cause_player picked up the ball",							this->BuildEvent(EventType::PickedUpTheBall,		75) },
		{ L"You picked up the ball",									this->BuildEvent(EventType::PickedUpTheBall,		75) },
		{ L"Ball dropped",												this->BuildEvent(EventType::BallDropped,			0) },
		{ L"You dropped the ball",										this->BuildEvent(EventType::BallDropped,			0) },
		{ L"Ball reset",												this->BuildEvent(EventType::BallReset,				0) },

		// Kill
		{ L"Killionaire!",												this->BuildEvent(EventType::Killionaire,			100) },
		{ L"Killpocalypse!",											this->BuildEvent(EventType::Killpocalypse,			90) },
		{ L"Killtastrophe!",											this->BuildEvent(EventType::Killtastrophe,			80) },
		{ L"Killimanjaro!",												this->BuildEvent(EventType::Killimanjaro,			70) },
		{ L"Killtrocity!",												this->BuildEvent(EventType::Killtrocity,			60) },
		{ L"Killtacular!",												this->BuildEvent(EventType::Killtacular,			50) },
		{ L"#cause_player stopped #effect_player",						this->BuildEvent(EventType::Stopped,				50) },
		{ L"You stopped #effect_player",								this->BuildEvent(EventType::Stopped,				50) }, // check this two
		{ L"#cause_player stopped you",									this->BuildEvent(EventType::Stopped,				50) }, // check this two
		{ L"Overkill!",													this->BuildEvent(EventType::Overkill,				40) },
		{ L"#cause_player stuck #effect_player",						this->BuildEvent(EventType::Stuck,					30) },
		{ L"#cause_player stuck you" ,									this->BuildEvent(EventType::Stuck,					30) },
		{ L"You stuck #effect_player",									this->BuildEvent(EventType::Stuck,					30) },
		{ L"#cause_player assassinated #effect_player",					this->BuildEvent(EventType::Assassinated,			30) },
		{ L"#effect_player was assassinated by #cause_player",			this->BuildEvent(EventType::Assassinated,			30) },
		{ L"#cause_player assassinated you",							this->BuildEvent(EventType::Assassinated,			30) },
		{ L"You assassinated #effect_player",							this->BuildEvent(EventType::Assassinated,			30) },
		{ L"You were killed while performing an assassination.",		this->BuildEvent(EventType::ShowStopper,			30) },
		{ L"Showstopper!",												this->BuildEvent(EventType::ShowStopper,			30) },
		{ L"Yoink!",													this->BuildEvent(EventType::Yoink,					30) },
		{ L"Triple Kill!",												this->BuildEvent(EventType::TripleKill,				30) },
		{ L"Double Kill!",												this->BuildEvent(EventType::DoubleKill,				20) },
		{ L"#cause_player beat down #effect_player",					this->BuildEvent(EventType::BeatDown,				20) },
		{ L"#cause_player beat you down",								this->BuildEvent(EventType::BeatDown,				20) },
		{ L"You beat down #effect_player",								this->BuildEvent(EventType::BeatDown,				20) },
		{ L"Headcase!",													this->BuildEvent(EventType::Headcase,				20) },
		{ L"Bulltrue!",													this->BuildEvent(EventType::Bulltrue,				20) },
		{ L"Splatter Spree!",											this->BuildEvent(EventType::SplatterSpree,			10) },
		{ L"Vehicular Manslaughter!",									this->BuildEvent(EventType::VehicularManslaughter,	10) },
		{ L"Sunday Driver!",											this->BuildEvent(EventType::SundayDriver,			10) },
		{ L"Sticky Spree!",												this->BuildEvent(EventType::StickySpree,			10) },
		{ L"Sticky Fingers!",											this->BuildEvent(EventType::StickyFingers,			10) },
		{ L"Corrected!",												this->BuildEvent(EventType::Corrected,				10) },
		{ L"Shotgun Spree!",											this->BuildEvent(EventType::ShotgunSpree,			10) },
		{ L"Open Season!",												this->BuildEvent(EventType::OpenSeason,				10) },
		{ L"Buck Wild!",												this->BuildEvent(EventType::BuckWild,				10) },
		{ L"Wrecking Crew!",											this->BuildEvent(EventType::WreckingCrew,			10) },
		{ L"Dreamcrusher!",												this->BuildEvent(EventType::Dreamcrusher,			10) },
		{ L"Hammer Spree!",												this->BuildEvent(EventType::HammerSpree,			10) },
		{ L"Sword Spree!",												this->BuildEvent(EventType::SwordSpree,				10) },
		{ L"Slice 'n Dice!",											this->BuildEvent(EventType::SliceNDice,				10) },
		{ L"Cutting Crew!", 											this->BuildEvent(EventType::CuttingCrew,			10) },

		// TODO: Get the other ones
		{ L"Juggernaut Spree!",											this->BuildEvent(EventType::JuggernautSpree,		10) },

		{ L"#cause_player's driving assisted you",						this->BuildEvent(EventType::Wheelman,				10) },
		{ L"Your driving assisted #effect_player",						this->BuildEvent(EventType::Wheelman,				10) },
		{ L"Wheelman Spree!",											this->BuildEvent(EventType::WheelmanSpree,			10) },
		{ L"Road Hog!",													this->BuildEvent(EventType::Roadhog,				10) },
		{ L"Road Rage!",												this->BuildEvent(EventType::Roadrage,				10) },
		{ L"#cause_player is on a Killing Spree!",						this->BuildEvent(EventType::KillingSpree,			10) },
		{ L"Killing Spree!",											this->BuildEvent(EventType::KillingSpree,			10) },
		{ L"#cause_player is on a Killing Frenzy!",						this->BuildEvent(EventType::KillingFrenzy,			10) },
		{ L"Killing Frenzy!",											this->BuildEvent(EventType::KillingFrenzy,			10) },
		{ L"#cause_player is a Running Riot!",							this->BuildEvent(EventType::RunningRiot,			10) },
		{ L"Running Riot!",												this->BuildEvent(EventType::RunningRiot,			10) },
		{ L"#cause_player is on a Rampage!",							this->BuildEvent(EventType::Rampage,				10) },
		{ L"Rampage!",													this->BuildEvent(EventType::Rampage,				10) },
		{ L"#cause_player is Untouchable!",								this->BuildEvent(EventType::Untouchable,			10) },
		{ L"Untouchable!",												this->BuildEvent(EventType::Untouchable,			10) },
		{ L"#cause_player is Invincible!",								this->BuildEvent(EventType::Invincible,				10) },
		{ L"Invincible!",												this->BuildEvent(EventType::Invincible,				10) },
		{ L"#cause_player is Inconceivable!",							this->BuildEvent(EventType::Inconceivable,			10) },
		{ L"Inconceivable!",											this->BuildEvent(EventType::Inconceivable,			10) },
		{ L"#cause_player is Unfrigginbelievable!",						this->BuildEvent(EventType::Unfrigginbelievable,	10) },
		{ L"Unfrigginbelievable!",										this->BuildEvent(EventType::Unfrigginbelievable,	10) },
		{ L"#cause_player splattered #effect_player",					this->BuildEvent(EventType::Splattered,				10) },
		{ L"You splattered #effect_player",								this->BuildEvent(EventType::Splattered,				10) },
		{ L"#cause_player splattered you",								this->BuildEvent(EventType::Splattered,				10) },
		{ L"#cause_player pummeled #effect_player",						this->BuildEvent(EventType::Pummeled,				10) },
		{ L"#cause_player pummeled you",								this->BuildEvent(EventType::Pummeled,				10) },
		{ L"You pummeled #effect_player",								this->BuildEvent(EventType::Pummeled,				10) },
		{ L"#cause_player killed #effect_player with a headshot",		this->BuildEvent(EventType::Headshot,				10) },
		{ L"#effect_player was killed by a #cause_player headshot",		this->BuildEvent(EventType::Headshot,				10) },
		{ L"#cause_player killed you with a headshot",					this->BuildEvent(EventType::Headshot,				10) },
		{ L"You killed #effect_player with a headshot",					this->BuildEvent(EventType::Headshot,				10) },
		{ L"#cause_player killed you from the grave",					this->BuildEvent(EventType::KillFromTheGrave,		5) },
		{ L"You killed #effect_player from the grave",					this->BuildEvent(EventType::KillFromTheGrave,		5) },
		{ L"#cause_player got revenge!",								this->BuildEvent(EventType::Revenge,				5) },
		{ L"Revenge!",													this->BuildEvent(EventType::Revenge,				5) },
		{ L"First Strike!",												this->BuildEvent(EventType::FirstStrike,			5) },
		{ L"Reload This!",												this->BuildEvent(EventType::ReloadThis,				5) },
		{ L"You were killed while reloading",							this->BuildEvent(EventType::ReloadThis,				5) },
		{ L"Close Call!",												this->BuildEvent(EventType::CloseCall,				5) },
		{ L"Protector!",												this->BuildEvent(EventType::Protector,				5) },
		{ L"#effect_player saved your life.",							this->BuildEvent(EventType::Protector,				5) },
		{ L"Firebird!",													this->BuildEvent(EventType::Firebird,				5) },
		{ L"Killjoy!",													this->BuildEvent(EventType::Killjoy,				5) },
		{ L"#cause_player ended your spree",							this->BuildEvent(EventType::Killjoy,				5) },
		{ L"Avenger!",													this->BuildEvent(EventType::Avenger,				5) },
		{ L"#effect_player avenged your death.",						this->BuildEvent(EventType::Avenger,				5) },
		{ L"Pull!",														this->BuildEvent(EventType::Pull,					5) },
		{ L"#cause_player struck #effect_player down!",					this->BuildEvent(EventType::Struck,					5) },
		{ L"#cause_player struck you down!",							this->BuildEvent(EventType::Struck,					5) },
		{ L"#cause_player killed #effect_player with grenades",			this->BuildEvent(EventType::KillWithGrenades,		5) },
		{ L"#effect_player was killed by #cause_player with grenades",	this->BuildEvent(EventType::KillWithGrenades,		5) },
		{ L"You killed #effect_player with grenades",					this->BuildEvent(EventType::KillWithGrenades,		5) },
		{ L"#cause_player killed you with grenades",					this->BuildEvent(EventType::KillWithGrenades,		5) },
		{ L"#cause_player lasered #effect_player",						this->BuildEvent(EventType::Lasered,				5) },
		{ L"#effect_player was lasered by #cause_player",				this->BuildEvent(EventType::Lasered,				5) },
		{ L"You lasered #effect_player",								this->BuildEvent(EventType::Lasered,				5) },
		{ L"#cause_player lasered you",									this->BuildEvent(EventType::Lasered,				5) },
		{ L"#cause_player sniped #effect_player",						this->BuildEvent(EventType::Sniped,					5) },
		{ L"#cause_player sniped you",									this->BuildEvent(EventType::Sniped,					5) },
		{ L"You sniped #effect_player",									this->BuildEvent(EventType::Sniped,					5) },
		{ L"Second Gunman!",											this->BuildEvent(EventType::SecondGunman,			5) },
		{ L"Sidekick!",													this->BuildEvent(EventType::Sidekick,				5) },
		{ L"Assist Spree!",												this->BuildEvent(EventType::AssistSpree,			5) },
		{ L"Assist",													this->BuildEvent(EventType::Assist,					0) },
		{ L"#cause_player killed #effect_player",						this->BuildEvent(EventType::Kill,					0) },
		{ L"You killed #effect_player",									this->BuildEvent(EventType::Kill,					0) },
		{ L"#cause_player killed you",									this->BuildEvent(EventType::Kill,					0) },
		{ L"#cause_player committed suicide",							this->BuildEvent(EventType::Suicide,				0) },
		{ L"You committed suicide",										this->BuildEvent(EventType::Suicide,				0) },
		{ L"#cause_player betrayed #effect_player",						this->BuildEvent(EventType::Betrayal,				0) },
		{ L"You betrayed #effect_player",								this->BuildEvent(EventType::Betrayal,				0) },
		{ L"#cause_player betrayed you",								this->BuildEvent(EventType::Betrayal,				0) },
		{ L"Killed by the Guardians",									this->BuildEvent(EventType::KilledByTheGuardians,	0) },
		{ L"You fell to your death",									this->BuildEvent(EventType::FellToYourDeath,		0) },
	};

	g_pState->EventRegistry.SetEventRegistry(std::move(tempMap));
}

EventInfo EventRegistrySystem::BuildEvent(EventType type, int weight)
{
	return { type, weight, this->ResolveEventClass(type) };
}

EventClass EventRegistrySystem::ResolveEventClass(EventType type)
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