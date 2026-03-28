#include "pch.h"
#include "Core/Systems/Infrastructure/Engine/FormatSystem.h"
#include <unordered_map>

std::string FormatSystem::ToTimestamp(float totalSeconds)
{
    if (totalSeconds < 0) totalSeconds = 0;

    int totalInt = static_cast<int>(totalSeconds);
    int hours = totalInt / 3600;
    int minutes = (totalInt % 3600) / 60;
    int seconds = totalInt % 60;
    int milliseconds = static_cast<int>((totalSeconds - totalInt) * 100);

    auto pad = [](int val) { return (val < 10 ? "0" : "") + std::to_string(val); };

    std::string result;
    result.reserve(16);

    if (hours > 0)
    {
        result += pad(hours) + ":" + pad(minutes) + ":" + pad(seconds) + "." + pad(milliseconds);
    }
    else
    {
        result += pad(minutes) + ":" + pad(seconds) + "." + pad(milliseconds);
    }

    return result;
}


std::string FormatSystem::WStringToString(const std::wstring& wstr) 
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring FormatSystem::StringToWString(const std::string& str) 
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


const char* FormatSystem::GetEventClassName(EventClass eventClass)
{
    switch (eventClass)
    {
    case EventClass::Fallback:              return "Fall Back";
    case EventClass::Server:                return "Server";
    case EventClass::Match:                 return "Match";
    case EventClass::Custom:                return "Custom";
    case EventClass::CaptureTheFlag:        return "Capture The Flag";
    case EventClass::Assault:               return "Assault";
    case EventClass::Slayer:                return "Slayer";
    case EventClass::Juggernaut:            return "Juggernaut";
    case EventClass::Race:                  return "Race";
    case EventClass::KingOfTheHill:         return "King Of The Hill";
    case EventClass::Territories:           return "Territories";
    case EventClass::Infection:             return "Infection";
    case EventClass::Oddball:               return "Oddball";
    case EventClass::KillRelated:           return "Kill Related";
    default:                                return "Unknown";
    }
}

std::string FormatSystem::EventTypeToString(EventType type) 
{
    switch (type) 
    {
    // Server types
    case EventType::Join:				    return "Join";
    case EventType::Rejoin:				    return "Rejoin";
    case EventType::Quit:				    return "Quit";
    case EventType::Booted:                 return "Booted";
    case EventType::JoinedTeam:             return "Joined Team";

        // Match
    case EventType::TookLead:			    return "Took Lead";
    case EventType::LostLead:			    return "Lost Lead";
    case EventType::TiedLead:			    return "Tied Lead";
    case EventType::OneMinuteToWin:         return "One Minute ToWin";
    case EventType::ThirtySecondsToWin:     return "Thirty Seconds To Win";
    case EventType::OneMinuteRemaining:     return "One Minute Remaining";
    case EventType::TenSecondsRemaining:    return "Ten Seconds Remaining";
    case EventType::ThirtySecondsRemaining: return "Thirty Seconds Remaining";
    case EventType::RoundOver:			    return "Round Over";
    case EventType::GameOver:			    return "Game Over";
    case EventType::Wins:				    return "Wins";

        // Custom
    case EventType::Custom:                 return "Custom";

        // CTF
    case EventType::CaptureTheFlag:		    return "Capture The Flag";
    case EventType::FlagCaptured:	        return "Flag Captured";
    case EventType::FlagStolen:	            return "Flag Stolen";
    case EventType::FlagDropped:	        return "Flag Dropped";
    case EventType::FlagReseted:	        return "Flag Reseted";
    case EventType::FlagRecovered:	        return "Flag Recovered";

        // Slayer
    case EventType::Slayer:		            return "Slayer";

        // Race
    case EventType::Race:                   return "Race";

        // Assault
    case EventType::Assault:		        return "Assault";
    case EventType::HasTheBomb:	            return "Has The Bomb";
    case EventType::ArmedTheBomb:	        return "Armed The Bomb";
    case EventType::DroppedTheBomb:	        return "Dropped The Bomb";
    case EventType::TeamScored:	            return "Team Scored";
    case EventType::BombReset:	            return "Bomb Reset";

        // Juggernaut
    case EventType::Juggernaut:             return "Juggernaut";
    case EventType::IsTheJuggernaut:		return "Is The Juggernaut";
    case EventType::YouKilledTheJuggernaut:	return "You Killed The Juggernaut";

        // KOTH
    case EventType::KingOfTheHill:		    return "King Of The Hill";
    case EventType::ControlsTheHill:	    return "Controls The Hill";
    case EventType::HillContested:	        return "Hill Contested";
    case EventType::HillMoved:	            return "Hill Moved";

        // Territories
    case EventType::Territories:		    return "Territories";
    case EventType::TeamCapturedATerritory:	return "Team Captured A Territory";
    case EventType::TerritoryContested:		return "Territory Contested";
    case EventType::TerritoryLost:		    return "Territory Lost";

        // Infection
    case EventType::Infection:		        return "Infection";
    case EventType::YouAreAZombie:		    return "You Are A Zombie";
    case EventType::Infected:	            return "Infected";
    case EventType::IsTheLastManStanding:	return "Is The Last Man Standing";
    case EventType::ZombiesWin:	            return "Zombies Win";
    case EventType::SurvivorsWin:	        return "Survivors Win";

        // Oddball
    case EventType::Oddball:		        return "Oddball";
    case EventType::PlayBall:	            return "Play Ball";
    case EventType::PickedUpTheBall:	    return "Picked Up The Ball";
    case EventType::BallDropped:	        return "Ball Dropped";
    case EventType::BallReset:	            return "Ball Reset";

        // Kill
    case EventType::Killionaire:			return "Killionaire";
    case EventType::Killpocalypse:			return "Killpocalypse";
    case EventType::Killtastrophe:			return "Killtastrophe";
    case EventType::Killimanjaro:			return "Killimanjaro";
    case EventType::Killtrocity:			return "Killtrocity";
    case EventType::Killtacular:			return "Killtacular";
    case EventType::Stopped:			    return "Stopped";
    case EventType::Overkill:			    return "Overkill";
    case EventType::Stuck:			        return "Stuck";
    case EventType::Assassinated:			return "Assassinated";
    case EventType::ShowStopper:			return "ShowStopper";
    case EventType::Yoink:			        return "Yoink";
    case EventType::TripleKill:			    return "Triple Kill";
    case EventType::DoubleKill:			    return "Double Kill";
    case EventType::BeatDown:			    return "Beat Down";
    case EventType::Headcase:			    return "Headcase";
    case EventType::Bulltrue:			    return "Bulltrue";
    case EventType::SplatterSpree:		    return "Splatter Spree";
    case EventType::VehicularManslaughter:	return "Vehicular Manslaughter";
    case EventType::SundayDriver:			return "Sunday Driver";
    case EventType::StickySpree:			return "Sticky Spree";
    case EventType::StickyFingers:			return "Sticky Fingers";
    case EventType::Corrected:			    return "Corrected";
    case EventType::ShotgunSpree:           return "Shotgun Spree";
    case EventType::OpenSeason:			    return "Open Season";
    case EventType::BuckWild:		        return "Buck Wild";
    case EventType::Dreamcrusher:		    return "Dreamcrusher";
    case EventType::WreckingCrew:			return "Wrecking Crew";
    case EventType::HammerSpree:		    return "Hammer Spree";
    case EventType::SwordSpree:			    return "Sword Spree";
    case EventType::SliceNDice:			    return "Slice N'Dice";
    case EventType::CuttingCrew:			return "Cutting Crew";
    case EventType::JuggernautSpree:		return "Juggernaut Spree";
    case EventType::Wheelman:		        return "Wheelman";
    case EventType::WheelmanSpree:	        return "Wheelman Spree";
    case EventType::Roadhog:	            return "Roadhog";
    case EventType::Roadrage:	            return "Roadrage";
    case EventType::KillingSpree:		    return "Killing Spree";
    case EventType::KillingFrenzy:		    return "Killing Frenzy";
    case EventType::RunningRiot:		    return "Running Riot";
    case EventType::Rampage:		        return "Rampage";
    case EventType::Untouchable:			return "Untouchable";
    case EventType::Invincible:			    return "Invincible";
    case EventType::Inconceivable:			return "Inconceivable";
    case EventType::Unfrigginbelievable:    return "Unfrigginbelievable";
    case EventType::Splattered:			    return "Splattered";
    case EventType::Pummeled:			    return "Pummeled";
    case EventType::Headshot:			    return "Headshot";
    case EventType::KillFromTheGrave:		return "Kill From The Grave";
    case EventType::Revenge:			    return "Revenge";
    case EventType::FirstStrike:		    return "First Strike";
    case EventType::ReloadThis:			    return "Reload This";
    case EventType::CloseCall:			    return "Close Call";
    case EventType::Protector:		        return "Protector";
    case EventType::Firebird:		        return "Firebird";
    case EventType::Killjoy:			    return "Killjoy";
    case EventType::Avenger:			    return "Avenger";
    case EventType::Pull:				    return "Pull";
    case EventType::Struck:				    return "Struck";
    case EventType::KillWithGrenades:		return "Kill With Grenades";
    case EventType::Lasered:		        return "Lasered";
    case EventType::Sniped:		            return "Sniped";
    case EventType::SecondGunman:	        return "Second Gunman";
    case EventType::Sidekick:	            return "Sidekick";
    case EventType::AssistSpree:	        return "Assist Spree";
    case EventType::Assist:				    return "Assist";
    case EventType::Kill:				    return "Kill";
    case EventType::Betrayal:			    return "Betrayal";
    case EventType::Suicide:			    return "Suicide";
    case EventType::KilledByTheGuardians:	return "Killed By The Guardians";
    case EventType::FellToYourDeath:	    return "Fell To Your Death";
    case EventType::SpawnSpree:	            return "Spawn Spree";
    case EventType::Wingman:	            return "Wingman";
    case EventType::Broseidon:	            return "Broseidon";

    case EventType::Ignore:			        return "Ignore";
    default:							return "Unknown";
    }
}

const std::unordered_map<EventType, EventMetadata>& FormatSystem::GetEventMetadata()
{
    static const std::unordered_map<EventType, EventMetadata> db = {
        // Server
        { EventType::Join,
            { "This event is called when a player enters the server for the first time.",
            { L"#cause_player joined the game" } }
        },
        { EventType::Rejoin,
            { "This event is called when a player re-enters the server.",
            { L"#cause_player rejoined the game" } }
        },
        { EventType::Quit,
            { "This event is called when a player quits the server.",
            { L"#cause_player quit" } }
        },
        { EventType::Booted,
            { "This event is called when a player is kicked or banned from the server.",
            { L"#effect_player was booted" } }
        },
        { EventType::JoinedTeam,
            { "This event is called when a player joins a team.",
            { L"#cause_player joined the #effect_team", L"#cause_player joined your team", L"You joined the #effect_team" } }
        },

        // Match
        { EventType::TookLead,
            { "This event is called when a team or a player tooks the lead.",
            { L"#cause_team took the lead!", L"#cause_player took the lead!", L"Your team took the lead!", L"You took the lead!" } }
        },
        { EventType::LostLead,
            { "This event is called when a team or a player loses the lead.",
            { L"You lost the lead", L"Your team lost the lead" } }
        },
        { EventType::TiedLead,
            { "This event is called when a team or a player ties the lead.",
            { L"#cause_team tied for the lead!", L"You are tied for the lead!", L"Your team is tied for the lead!", L"#cause_player tied for the lead!" } }
        },
         { EventType::TeamScored,
            { "This event is called when a team scores.",
            { L"#cause_team scored", } }
        },
        { EventType::OneMinuteToWin,
            { "This event is called when there is one minute left to win.",
            { L"1 minute to win!", L"#cause_player 1 minute to win!" } }
        },
        { EventType::ThirtySecondsToWin,
            { "This event is called when there is one minute left to win.",
            { L"30 seconds to win!", L"#cause_player 30 seconds to win!" } }
        },
        { EventType::OneMinuteRemaining,
            { "This event is called when there is one minute to end the game",
            { L"1 minute remaining" } }
        },
        { EventType::ThirtySecondsRemaining,
            { "This event is called when there is 30 seconds to end the game",
            { L"30 seconds remaining" } }
        },
        { EventType::TenSecondsRemaining,
            { "This event is called when there is 10 seconds to end the game",
            { L"10 seconds remaining" } }
        },
        { EventType::RoundOver,
            { "This event is called when a round is over.",
            { L"Round over" } }
        },
        { EventType::GameOver,
            { "This event is called when the game is over.",
            { L"Game over" } }
        },

        // Custom
        { EventType::Custom,
            { "This event is called at the start of the game, when the gametype is custom.",
            { L"Custom" } }
        },

        // CTF
        { EventType::CaptureTheFlag,
            { "This event is called at the start of the game, when the gametype is CTF.",
            { L"Capture the Flag" } }
        },
        { EventType::FlagCaptured,
            { "This event is called when a player captures the enemy team flag.",
            { L"#cause_team captured a flag!", L"You captured a flag!" } }
        },
        { EventType::FlagStolen,
            { "This event is called when a player steals the enemy team flag.",
            { L"#effect_team flag was stolen!", L"Your flag has been stolen!", L"You grabbed the #effect_team flag!" } }
        },
        { EventType::FlagDropped,
            { "This event is called when a player drops the enemy team flag.",
            { L"#effect_team flag dropped", L"You dropped #effect_team flag" } }
        },
        { EventType::FlagReseted,
            { "This event is called when a flag is reseted.",
            { L"#effect_team flag reset" } }
        },
        { EventType::FlagRecovered,
            { "This event is called when a flag is recovered.",
            { L"#effect_team flag recovered", L"Your flag has been recovered" } }
        },

        // Slayer
        { EventType::Slayer,
            { "This event is called at the start of the game, when the gametype is Slayer.",
            { L"Slayer" } }
        },

        // Race
        { EventType::Race,
            { "This event is called at the start of the game, when the gametype is Race.",
            { L"Race" } }
        },

        // Assault
        { EventType::Assault,
            { "This event is called at the start of the game, when the gametype is Assault.",
            { L"Assault" } }
        },
        { EventType::HasTheBomb,
            { "This event is called when a player picks the bomb.",
            { L"You picked up the bomb" } }
        },
        { EventType::ArmedTheBomb,
            { "This event is called when a player arms the bomb.",
            { L"You armed the bomb", L"#cause_player armed the bomb" } }
        },
        { EventType::DroppedTheBomb,
            { "This event is called when a player drops the bomb.",
            { L"You dropped the bomb" } }
        },
        { EventType::BombReset,
            { "This event is called when the bomb resets.",
            { L"Bomb reset" } }
        },

        // Juggernaut
        { EventType::Juggernaut,
            { "This event is called at the start of the game, when the gametype is Juggernaut.",
            { L"Juggernaut" } }
        },
        { EventType::IsTheJuggernaut,
            { "This event is called when a player becomes the juggernaut.",
            { L"You are the juggernaut", L"#cause_player is the juggernaut" } }
        },
        { EventType::YouKilledTheJuggernaut,
            { "This event is called when a player kills the juggernaut.",
            { L"You killed the juggernaut" } }
        },

        // KOTH
        { EventType::KingOfTheHill,
            { "This event is called at the start of the game, when the gametype is KOTH.",
            { L"King of the Hill" } }
        },
        { EventType::ControlsTheHill,
            { "This event is called when a player or a team controls a hill.",
            { L"#cause_player controls the hill", L"You control the hill", L"Your team controls the hill" } }
        },
        { EventType::HillContested,
            { "This event is called when a hill is contested.",
            { L"Hill contested" } }
        },
        { EventType::HillMoved,
            { "This event is called when a hill has moved.",
            { L"Hill moved" } }
        },

        // Territories
        { EventType::Territories,
            { "This event is called at the start of the game, when the gametype is Territories.",
            { L"Territories" } }
        },
        { EventType::TeamCapturedATerritory,
            { "This event is called when a team captures a territory.",
            { L"#cause_team captured a territory", L"Your team captured a territory" } }
        },
        { EventType::TerritoryContested,
            { "This event is called when a territory is contested.",
            { L"Territory contested" } }
        },
        { EventType::TerritoryLost,
            { "This event is called when a team loses a territory.",
            { L"Territory lost" } }
        },

        // Infection
        { EventType::Infection,
            { "This event is called at the start of the game, when the gametype is Infection.",
            { L"Infection" } }
        },
        { EventType::YouAreAZombie,
            { "This event is called when a player turns into a zombie.",
            { L"You are a zombie" } }
        },
        { EventType::Infected,
            { "This event is called when a player infects another player.",
            { L"You infected #effect_player", L"#cause_player infected you" } }
        },
        { EventType::IsTheLastManStanding,
            { "This event is called when a player is the last survivor.",
            { L"#cause_player is the last man standing", L"You are the last man standing" } }
        },
        { EventType::ZombiesWin,
            { "This event is called when a the zombies wins.",
            { L"Zombies Win!" } }
        },
        { EventType::SurvivorsWin,
            { "This event is called when a the survivors wins.",
            { L"Survivors Win!" } }
        },

        // Oddball
        { EventType::Oddball,
            { "This event is called at the start of the game, when the gametype is Oddball.",
            { L"Oddball" } }
        },
        { EventType::PlayBall,
            { "This event is called when a oddball spawns.",
            { L"Play ball!" } }
        },
        { EventType::PickedUpTheBall,
            { "This event is called when a player picks the oddball.",
            { L"#cause_player picked up the ball", L"You picked up the ball" } }
        },
        { EventType::BallDropped,
            { "This event is called when a player drops the oddball.",
            { L"Ball dropped", L"You dropped the ball" } }
        },
        { EventType::BallReset,
            { "This event is called when a the oddball resets.",
            { L"Ball reset" } }
        },

        // Kill
        { EventType::Killionaire,
            { "This event is called when a player kills 10 opponents within a 4-second window since last kill.",
            { L"Killionaire!" } }
        },
        { EventType::Killpocalypse,
            { "This event is called when a player kills 9 opponents within a 4-second window since last kill.",
            { L"Killpocalypse!" } }
        },
        { EventType::Killtastrophe,
            { "This event is called when a player kills 8 opponents within a 4-second window since last kill.",
            { L"Killtastrophe!" } }
        },
        { EventType::Killimanjaro,
            { "This event is called when a player kills 7 opponents within a 4-second window since last kill.",
            { L"Killimanjaro!" } }
        },
        { EventType::Killtrocity,
            { "This event is called when a player kills 6 opponents within a 4-second window since last kill.",
            { L"Killtrocity!" } }
        },
        { EventType::Killtacular,
            { "This event is called when a player kills 5 opponents within a 4-second window since last kill.",
            { L"Killtacular!" } }
        },
        { EventType::Overkill,
            { "This event is called when a player kills 4 opponents within a 4-second window since last kill.",
            { L"Overkill!" } }
        },
        { EventType::TripleKill,
            { "This event is called when a player kills 3 opponents within a 4-second window since last kill.",
            { L"Triple Kill!" } }
        },
        { EventType::DoubleKill,
            { "This event is called when a player kills 2 opponents within a 4-second window since last kill.",
            { L"Double Kill!" } }
        },
        { EventType::Assassinated,
            { "This event is called when a player performs a cinematic melee execution from behind.",
            { L"#cause_player assassinated #effect_player", L"#effect_player was assassinated by #cause_player", L"#cause_player assassinated you", L"You assassinated #effect_player" } }
        },
        { EventType::ShowStopper,
            { "This event is called when a player kills an opponent who is currently performing an assassination animation.",
            { L"Showstopper!", L"You were killed while performing an assassination." } }
        },
        { EventType::Yoink,
            { "This event is called when a player steals a teammate's assassination kill by killing the victim first.",
            { L"Yoink!" } }
        },
        { EventType::BeatDown,
            { "This event is called when a player kills an opponent with a melee attack from behind.",
            { L"#cause_player beat down #effect_player", L"#cause_player beat you down", L"You beat down #effect_player" } }
        },
        { EventType::Bulltrue,
            { "This event is called when a player kills an opponent who is in the act of a Sword Lunge.",
            { L"Bulltrue!" } }
        },
        { EventType::Headcase,
            { "This event is called when a player kills a sprinting opponent with a headshot.",
            { L"Headcase!" } }
        },
        { EventType::Stopped,
            { "This event is called when a player ends an opponent's movement or objective progress.",
            { L"#cause_player stopped #effect_player", L"You stopped #effect_player", L"#cause_player stopped you" } }
        },
        { EventType::ShotgunSpree,
            { "This event is called when a player kills 5 opponents with a Shotgun without dying.",
            { L"Shotgun Spree!" } }
        },
        { EventType::OpenSeason,
            { "This event is called when a player kills 10 opponents with a Shotgun without dying.",
            { L"Open Season!" } }
        },
        { EventType::BuckWild,
            { "This event is called when a player kills 15 opponents with a Shotgun without dying.",
            { L"Buck Wild!" } }
        },
        { EventType::SwordSpree,
            { "This event is called when a player kills 5 opponents with an Energy Sword without dying.",
            { L"Sword Spree!" } }
        },
        { EventType::SliceNDice,
            { "This event is called when a player kills 10 opponents with an Energy Sword without dying.",
            { L"Slice 'n Dice!" } }
        },
        { EventType::CuttingCrew,
            { "This event is called when a player kills 15 opponents with an Energy Sword without dying.",
            { L"Cutting Crew!" } }
        },
        { EventType::HammerSpree,
            { "This event is called when a player kills 5 opponents with a Gravity Hammer without dying.",
            { L"Hammer Spree!" } }
        },
        { EventType::Dreamcrusher,
            { "This event is called when a player kills 10 opponents with a Gravity Hammer without dying.",
            { L"Dreamcrusher!" } }
        },
        { EventType::WreckingCrew,
            { "This event is called when a player kills 15 opponents with a Gravity Hammer without dying.",
            { L"Wrecking Crew!" } }
        },
        { EventType::Stuck,
            { "This event is called when a player kills an opponent by attaching a Plasma Grenade to them.",
            { L"#cause_player stuck #effect_player", L"#cause_player stuck you", L"You stuck #effect_player" } }
        },
        { EventType::StickySpree,
            { "This event is called when a player kills 5 opponents with grenade sticks without dying.",
            { L"Sticky Spree!" } }
        },
        { EventType::StickyFingers,
            { "This event is called when a player kills 10 opponents with grenade sticks without dying.",
            { L"Sticky Fingers!" } }
        },
        { EventType::Corrected,
            { "This event is called when a player kills 15 opponents with grenade sticks without dying.",
            { L"Corrected!" } }
        },
        { EventType::SplatterSpree,
            { "This event is called when a player splatters 5 opponents using a vehicle without dying.",
            { L"Splatter Spree!" } }
        },
        { EventType::VehicularManslaughter,
            { "This event is called when a player splatters 10 opponents using a vehicle without dying.",
            { L"Vehicular Manslaughter!" } }
        },
        { EventType::SundayDriver,
            { "This event is called when a player splatters 15 opponents using a vehicle without dying.",
            { L"Sunday Driver!" } }
        },
        { EventType::JuggernautSpree,
            { "This event is called when the Juggernaut kills 5 opponents without dying.",
            { L"Juggernaut Spree!" } }
        },
        { EventType::Wheelman,
            { "This event is called when a passenger kills an enemy while a player is driving the vehicle.",
            { L"#cause_player's driving assisted you", L"Your driving assisted #effect_player" } }
        },
        { EventType::WheelmanSpree,
            { "This event is called when passengers kill 5 enemies in a single life while a player is driving.",
            { L"Wheelman Spree!" } }
        },
        { EventType::Roadhog,
            { "This event is called when passengers kill 10 enemies in a single life while a player is driving.",
            { L"Road Hog!" } }
        },
        { EventType::Roadrage,
            { "This event is called when passengers kill 15 enemies in a single life while a player is driving.",
            { L"Road Rage!" } }
        },
        { EventType::KillingSpree,
            { "This event is called when a player kills 5 enemies without dying.",
            { L"#cause_player is on a Killing Spree!", L"Killing Spree!" } }
        },
        { EventType::KillingFrenzy,
            { "This event is called when a player kills 10 enemies without dying.",
            { L"#cause_player is on a Killing Frenzy!", L"Killing Frenzy!" } }
        },
        { EventType::RunningRiot,
            { "This event is called when a player kills 15 enemies without dying.",
            { L"#cause_player is a Running Riot!", L"Running Riot!" } }
        },
        { EventType::Rampage,
            { "This event is called when a player kills 20 enemies without dying.",
            { L"#cause_player is on a Rampage!", L"Rampage!" } }
        },
        { EventType::Untouchable,
            { "This event is called when a player kills 25 enemies without dying.",
            { L"#cause_player is Untouchable!", L"Untouchable!" } }
        },
        { EventType::Invincible,
            { "This event is called when a player kills 30 enemies without dying.",
            { L"#cause_player is Invincible!", L"Invincible!" } }
        },
        { EventType::Inconceivable,
            { "This event is called when a player kills 35 enemies without dying.",
            { L"#cause_player is Inconceivable!", L"Inconceivable!" } }
        },
        { EventType::Unfrigginbelievable,
            { "This event is called when a player kills 40 enemies without dying.",
            { L"#cause_player is Unfrigginbelievable!", L"Unfrigginbelievable!" } }
        },
        { EventType::Splattered,
            { "This event is called when a player is killed by being hit by a vehicle.",
            { L"#cause_player splattered #effect_player", L"You splattered #effect_player", L"#cause_player splattered you" } }
        },
        { EventType::Pummeled,
            { "This event is called when a player is killed with a basic melee attack.",
            { L"#cause_player pummeled #effect_player", L"#cause_player pummeled you", L"You pummeled #effect_player" } }
        },
        { EventType::Headshot,
            { "This event is called when a player is killed with a shot to the head.",
            { L"#cause_player killed #effect_player with a headshot", L"#effect_player was killed by a #cause_player headshot", L"#cause_player killed you with a headshot", L"You killed #effect_player with a headshot" } }
        },
        { EventType::KillFromTheGrave,
            { "This event is called when a player kills an enemy after having died.",
            { L"#cause_player killed you from the grave", L"You killed #effect_player from the grave" } }
        },
        { EventType::Revenge,
            { "This event is called when a player kills the last opponent who killed them.",
            { L"#cause_player got revenge!", L"Revenge!" } }
        },
        { EventType::FirstStrike,
            { "This event is called when the first kill of the match is secured.",
            { L"First Strike!" } }
        },
        { EventType::ReloadThis,
            { "This event is called when a player kills an enemy who was in the middle of reloading.",
            { L"Reload This!", L"You were killed while reloading" } }
        },
        { EventType::CloseCall,
            { "This event is called when a player kills an enemy while their own health is critically low.",
            { L"Close Call!" } }
        },
        { EventType::Protector,
            { "This event is called when a player kills an enemy who was damaging a teammate.",
            { L"Protector!", L"#effect_player saved your life." } }
        },
        { EventType::Firebird,
            { "This event is called when a player kills an enemy while the attacker is using a Jetpack.",
            { L"Firebird!" } }
        },
        { EventType::Killjoy,
            { "This event is called when a player ends an opponent's spree.",
            { L"Killjoy!", L"#cause_player ended your spree" } }
        },
        { EventType::Avenger,
            { "This event is called when a player kills an enemy who recently killed a teammate.",
            { L"Avenger!", L"#effect_player avenged your death." } }
        },
        { EventType::Pull,
            { "This event is called when a player kills an opponent who is currently using a Jetpack.",
            { L"Pull!" } }
        },
        { EventType::Struck,
            { "It seems related to First Strike event.",
            { L"#cause_player struck #effect_player down!", L"#cause_player struck you down!" } }
        },
        { EventType::KillWithGrenades,
            { "This event is called when a player kills an opponent using fragmentation or plasma grenades.",
            { L"#cause_player killed #effect_player with grenades", L"#effect_player was killed by #cause_player with grenades", L"You killed #effect_player with grenades", L"#cause_player killed you with grenades" } }
        },
        { EventType::Lasered,
            { "This event is called when a player is killed by a Spartan Laser.",
            { L"#cause_player lasered #effect_player", L"#effect_player was lasered by #cause_player", L"You lasered #effect_player", L"#cause_player lasered you" } }
        },
        { EventType::Sniped,
            { "This event is called when a player is killed with a sniper rifle.",
            { L"#cause_player sniped #effect_player", L"#cause_player sniped you", L"You sniped #effect_player" } }
        },
        { EventType::SecondGunman,
            { "This event is called when a player assists in 15 kills without dying.",
            { L"Second Gunman!" } }
        },
        { EventType::Sidekick,
            { "This event is called when a player assists in 10 kills without dying.",
            { L"Sidekick!" } }
        },
        { EventType::AssistSpree,
            { "This event is called when a player assists in 5 kills without dying.",
            { L"Assist Spree!" } }
        },
        { EventType::Assist,
            { "This event is called when a player damages an enemy who is then killed by a teammate.",
            { L"Assist" } }
        },
        { EventType::Kill,
            { "This event is called when a player kills an enemy.",
            { L"#cause_player killed #effect_player", L"You killed #effect_player", L"#cause_player killed you" } }
        },
        { EventType::Suicide,
            { "This event is called when a player causes their own death.",
            { L"#cause_player committed suicide", L"You committed suicide" } }
        },
        { EventType::Betrayal,
            { "This event is called when a player kills a member of their own team.",
            { L"#cause_player betrayed #effect_player", L"You betrayed #effect_player", L"#cause_player betrayed you" } }
        },
        { EventType::KilledByTheGuardians,
            { "This event is called when a player is killed by world hazards or unknown map elements.",
            { L"Killed by the Guardians" } }
        },
        { EventType::FellToYourDeath,
            { "This event is called when a player dies by falling off the map boundaries.",
            { L"You fell to your death" } }
        },
    };

    return db;
}