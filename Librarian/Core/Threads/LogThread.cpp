#include "pch.h"
#include "LogThread.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Systems/Theater.h"
#include "Core/Systems/Director.h"
#include "Core/Systems/Timeline.h"
#include "Hooks/Lifecycle/GameEngineStart_Hook.h"

std::thread g_LogThread;
bool g_LogGameEvents = false;

std::string LogThread::EventTypeToString(EventType type) {
    switch (type) {
        // Server types
        case EventType::Join:				    return "Join";
        case EventType::Rejoin:				    return "Rejoin";
        case EventType::Quit:				    return "Quit";
        case EventType::Booted:                 return "Booted";
        case EventType::MinutesRemaining:       return "MinutesRemaining";
        case EventType::TenSecondsRemaining:    return "TenSecondsRemaining";
        case EventType::ThirtySecondsRemaining: return "ThirtySecondsRemaining";

        // Match
        case EventType::TookLead:			    return "TookLead";
        case EventType::TiedLead:			    return "TiedLead";
        case EventType::GameOver:			    return "GameOver";
        case EventType::Wins:				    return "Wins";

        // CTF
        case EventType::CaptureTheFlag:		    return "CaptureTheFlag";
        case EventType::FlagStolen:	            return "FlagStolen";
        case EventType::FlagCaptured:	        return "FlagCaptured";
        case EventType::FlagReseted:	        return "FlagReseted";
        case EventType::FlagRecovered:	        return "FlagRecovered";
        case EventType::FlagDropped:	        return "FlagDropped";
        case EventType::Stopped:			    return "Stopped";

        // Assault
        case EventType::Assault:		        return "Assault";
        case EventType::HasTheBomb:	            return "HasTheBomb";
        case EventType::DroppedTheBomb:	        return "DroppedTheBomb";
        case EventType::ArmedTheBomb:	        return "ArmedTheBomb";
        case EventType::TeamScored:	            return "TeamScored";
        case EventType::BombReset:	            return "BombReset";

        // Slayer
        case EventType::Slayer:		            return "Slayer";

        // Juggernaut
        case EventType::Juggernaut:             return "Juggernaut";
        case EventType::IsTheJuggernaut:		return "IsTheJuggernaut";

        // Race
        case EventType::Race:                   return "Race";

        // KOTH
        case EventType::KingOfTheHill:		    return "KingOfTheHill";
        case EventType::ControlsTheHill:	    return "ControlsTheHill";
        case EventType::HillContested:	        return "HillContested";
        case EventType::OneMinuteToWin:	        return "OneMinuteToWin";
        case EventType::ThirtySecondsToWin:	    return "ThirtySecondsToWin";

        // Territories
        case EventType::Territories:		    return "Territories";
        case EventType::TeamCapturedATerritory:	return "TeamCapturedATerritory";
        case EventType::TerritoryContested:		return "TerritoryContested";
        case EventType::TerritoryLost:		    return "TerritoryLost";

        // Infection
        case EventType::Infection:		        return "Infection";
        case EventType::Infected:	            return "Infected";
        case EventType::IsTheLastManStanding:	return "IsTheLastManStanding";
        case EventType::ZombiesWin:	            return "ZombiesWin";
        case EventType::SurvivorsWin:	        return "SurvivorsWin";

        // Oddball
        case EventType::Oddball:		        return "Oddball";
        case EventType::PlayBall:	            return "PlayBall";
        case EventType::BallReset:	            return "BallReset";
        case EventType::PickedUpTheBall:	    return "PickedUpTheBall";
        case EventType::BallDropped:	        return "BallDropped";

        // Medal types
        case EventType::Headshot:			    return "Headshot";
        case EventType::Pummeled:			    return "Pummeled";
        case EventType::Stuck:				    return "Stuck";
        case EventType::KillingSpree:		    return "KillingSpree";
        case EventType::Splattered:			    return "Splattered";
        case EventType::BeatDown:			    return "BeatDown";
        case EventType::Assassinated:		    return "Assassinated";
        case EventType::KillingFrenzy:		    return "KillingFrenzy";
        case EventType::RunningRiot:		    return "RunningRiot";
        case EventType::Lasered:		        return "Lasered";
        case EventType::Sniped:		            return "Sniped";
        case EventType::Rampage:		        return "Rampage";

        // Special types
        case EventType::Struck:				    return "Struck";
        case EventType::DoubleKill:			    return "DoubleKill";
        case EventType::TripleKill:			    return "TripleKill";
        case EventType::SwordSpree:			    return "SwordSpree";
        case EventType::ReloadThis:			    return "ReloadThis";
        case EventType::Revenge:			    return "Revenge";
        case EventType::Killjoy:			    return "Killjoy";
        case EventType::Avenger:			    return "Avenger";
        case EventType::Pull:				    return "Pull";
        case EventType::Headcase:			    return "Headcase";
        case EventType::Assist:				    return "Assist";
        case EventType::CloseCall:			    return "CloseCall";
        case EventType::FirstStrike:		    return "FirstStrike";
        case EventType::Firebird:		        return "Firebird";
        case EventType::Protector:		        return "Protector";
        case EventType::Overkill:		        return "Overkill";
        case EventType::HammerSpree:		    return "HammerSpree";

        case EventType::SplatterSpree:		    return "SplatterSpree";
        case EventType::VehicularManslaughter:	return "VehicularManslaughter";
        case EventType::SundayDriver:			return "SundayDriver";
        case EventType::SliceNDice:			    return "SliceNDice";
        case EventType::CuttingCrew:			return "CuttingCrew";
        case EventType::StickySpree:			return "StickySpree";
        case EventType::StickyFingers:			return "StickyFingers";
        case EventType::Corrected:			    return "Corrected";
        case EventType::Dreamcrusher:		    return "Dreamcrusher";
        case EventType::WreckingCrew:			return "WreckingCrew";
        case EventType::ShotgunSpree:           return "ShotgunSpree";
        case EventType::OpenSeason:			    return "OpenSeason";
        case EventType::BuckWild:		        return "BuckWild";
        case EventType::Killtacular:		    return "Killtacular";
        case EventType::Killtrocity:		    return "Killtrocity";
        case EventType::Killimanjaro:		    return "Killimanjaro";
        case EventType::Killtastrophe:		    return "Killtastrophe";

        case EventType::Killpocalypse:		    return "Killpocalypse";
        case EventType::Killionaire:	        return "Killionaire";
        case EventType::Bulltrue:			    return "Bulltrue";
        case EventType::Untouchable:			return "Untouchable";
        case EventType::Invincible:			    return "Invincible";
        case EventType::Inconceivable:			return "Inconceivable";
        case EventType::Unfrigginbelievable:    return "Unfrigginbelievable";

        // Death types
        case EventType::Kill:				    return "Kill";
        case EventType::Betrayal:			    return "Betrayal";
        case EventType::Suicide:			    return "Suicide";

        case EventType::Ignore:			        return "Ignore";
        default:							return "Unknown";
    }
}

void LogThread::Run()
{
    Logger::LogAppend("=== Log Thread Started ===");

    static size_t processedCount = 0;

    while (g_Running)
    {
        if (!g_LogGameEvents || !g_IsTheaterMode) continue;

        while (processedCount < g_Timeline.size())
        {
            const auto& gameEvent = g_Timeline[processedCount];

            std::stringstream ss;
            ss << "[" << Formatting::ToTimestamp(gameEvent.Timestamp) << "] ";
            ss << EventTypeToString(gameEvent.Type);

            if (!gameEvent.Players.empty()) {
                ss << ": ";
                for (size_t i = 0; i < gameEvent.Players.size(); ++i) {
                    ss << gameEvent.Players[i].Name;
                    if (i < gameEvent.Players.size() - 1) {
                        ss << ", ";
                    }
                }
            }

            Logger::LogAppend(ss.str().c_str());
            processedCount++;
        }

        if (processedCount > g_Timeline.size())
        {
            processedCount = 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    Logger::LogAppend("=== Log Thread Stopped ===");
}