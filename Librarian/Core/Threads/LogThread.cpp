#include "pch.h"
#include "LogThread.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Common/GlobalState.h"
#include "Hooks/Lifecycle/GameEngineStart_Hook.h"
#include <sstream>
#include <chrono>

using namespace std::chrono_literals;

std::thread g_LogThread;

std::string LogThread::EventTypeToString(EventType type) {
    switch (type) {
        // Server types
        case EventType::Join:				    return "Join";
        case EventType::Rejoin:				    return "Rejoin";
        case EventType::Quit:				    return "Quit";
        case EventType::Booted:                 return "Booted";
        case EventType::JoinedTeam:             return "JoinedTeam";

        // Match
        case EventType::TookLead:			    return "TookLead";
        case EventType::LostLead:			    return "LostLead";
        case EventType::TiedLead:			    return "TiedLead";
        case EventType::OneMinuteToWin:         return "OneMinuteToWin";
        case EventType::ThirtySecondsToWin:     return "ThirtySecondsToWin";
        case EventType::OneMinuteRemaining:     return "OneMinuteRemaining";
        case EventType::TenSecondsRemaining:    return "TenSecondsRemaining";
        case EventType::ThirtySecondsRemaining: return "ThirtySecondsRemaining";
        case EventType::RoundOver:			    return "RoundOver";
        case EventType::GameOver:			    return "GameOver";
        case EventType::Wins:				    return "Wins";

        // Custom
        case EventType::Custom:                 return "Custom";

        // CTF
        case EventType::CaptureTheFlag:		    return "CaptureTheFlag";
        case EventType::FlagCaptured:	        return "FlagCaptured";
        case EventType::FlagStolen:	            return "FlagStolen";
        case EventType::FlagDropped:	        return "FlagDropped";
        case EventType::FlagReseted:	        return "FlagReseted";
        case EventType::FlagRecovered:	        return "FlagRecovered";

        // Slayer
        case EventType::Slayer:		            return "Slayer";

        // Race
        case EventType::Race:                   return "Race";

        // Assault
        case EventType::Assault:		        return "Assault";
        case EventType::HasTheBomb:	            return "HasTheBomb";
        case EventType::ArmedTheBomb:	        return "ArmedTheBomb";
        case EventType::DroppedTheBomb:	        return "DroppedTheBomb";
        case EventType::TeamScored:	            return "TeamScored";
        case EventType::BombReset:	            return "BombReset";

        // Juggernaut
        case EventType::Juggernaut:             return "Juggernaut";
        case EventType::IsTheJuggernaut:		return "IsTheJuggernaut";
        case EventType::YouKilledTheJuggernaut:	return "YouKilledTheJuggernaut";

        // KOTH
        case EventType::KingOfTheHill:		    return "KingOfTheHill";
        case EventType::ControlsTheHill:	    return "ControlsTheHill";
        case EventType::HillContested:	        return "HillContested";
        case EventType::HillMoved:	            return "HillMoved";

        // Territories
        case EventType::Territories:		    return "Territories";
        case EventType::TeamCapturedATerritory:	return "TeamCapturedATerritory";
        case EventType::TerritoryContested:		return "TerritoryContested";
        case EventType::TerritoryLost:		    return "TerritoryLost";

        // Infection
        case EventType::Infection:		        return "Infection";
        case EventType::YouAreAZombie:		    return "YouAreAZombie";
        case EventType::Infected:	            return "Infected";
        case EventType::IsTheLastManStanding:	return "IsTheLastManStanding";
        case EventType::ZombiesWin:	            return "ZombiesWin";
        case EventType::SurvivorsWin:	        return "SurvivorsWin";

        // Oddball
        case EventType::Oddball:		        return "Oddball";
        case EventType::PlayBall:	            return "PlayBall";
        case EventType::PickedUpTheBall:	    return "PickedUpTheBall";
        case EventType::BallDropped:	        return "BallDropped";
        case EventType::BallReset:	            return "BallReset";

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
        case EventType::TripleKill:			    return "TripleKill";
        case EventType::DoubleKill:			    return "DoubleKill";
        case EventType::BeatDown:			    return "BeatDown";
        case EventType::Headcase:			    return "Headcase";
        case EventType::Bulltrue:			    return "Bulltrue";
        case EventType::SplatterSpree:		    return "SplatterSpree";
        case EventType::VehicularManslaughter:	return "VehicularManslaughter";
        case EventType::SundayDriver:			return "SundayDriver";
        case EventType::StickySpree:			return "StickySpree";
        case EventType::StickyFingers:			return "StickyFingers";
        case EventType::Corrected:			    return "Corrected";
        case EventType::ShotgunSpree:           return "ShotgunSpree";
        case EventType::OpenSeason:			    return "OpenSeason";
        case EventType::BuckWild:		        return "BuckWild";
        case EventType::Dreamcrusher:		    return "Dreamcrusher";
        case EventType::WreckingCrew:			return "WreckingCrew";
        case EventType::HammerSpree:		    return "HammerSpree";
        case EventType::SwordSpree:			    return "SwordSpree";
        case EventType::SliceNDice:			    return "SliceNDice";
        case EventType::CuttingCrew:			return "CuttingCrew";
        case EventType::JuggernautSpree:		return "JuggernautSpree";
        case EventType::Wheelman:		        return "Wheelman";
        case EventType::KillingSpree:		    return "KillingSpree";
        case EventType::KillingFrenzy:		    return "KillingFrenzy";
        case EventType::RunningRiot:		    return "RunningRiot";
        case EventType::Rampage:		        return "Rampage";
        case EventType::Untouchable:			return "Untouchable";
        case EventType::Invincible:			    return "Invincible";
        case EventType::Inconceivable:			return "Inconceivable";
        case EventType::Unfrigginbelievable:    return "Unfrigginbelievable";
        case EventType::Splattered:			    return "Splattered";
        case EventType::Pummeled:			    return "Pummeled";
        case EventType::Headshot:			    return "Headshot";
        case EventType::KillFromTheGrave:		return "KillFromTheGrave";
        case EventType::Revenge:			    return "Revenge";
        case EventType::FirstStrike:		    return "FirstStrike";
        case EventType::ReloadThis:			    return "ReloadThis";
        case EventType::CloseCall:			    return "CloseCall";
        case EventType::Protector:		        return "Protector";
        case EventType::Firebird:		        return "Firebird";
        case EventType::Killjoy:			    return "Killjoy";
        case EventType::Avenger:			    return "Avenger";
        case EventType::Pull:				    return "Pull";
        case EventType::Struck:				    return "Struck";
        case EventType::KillWithGrenades:		return "KillWithGrenades";
        case EventType::Lasered:		        return "Lasered";
        case EventType::Sniped:		            return "Sniped";
        case EventType::Assist:				    return "Assist";
        case EventType::Kill:				    return "Kill";
        case EventType::Betrayal:			    return "Betrayal";
        case EventType::Suicide:			    return "Suicide";
        case EventType::KilledByTheGuardians:	return "KilledByTheGuardians";
        case EventType::FellToYourDeath:	    return "FellToYourDeath";

        case EventType::Ignore:			        return "Ignore";
        default:							return "Unknown";
    }
}

void LogThread::Run()
{
    Logger::LogAppend("=== Log Thread Started ===");

    while (g_State.running.load())
    {
        if (!g_State.logGameEvents.load() || !g_State.isTheaterMode.load()) {
            std::this_thread::sleep_for(100ms);
            continue;
        }

        std::vector<GameEvent> eventsToProcess;

        {
            std::lock_guard<std::mutex> lock(g_State.timelineMutex);

            size_t currentSize = g_State.timeline.size();
            size_t lastProcessed = g_State.processedCount.load();

            if (lastProcessed < currentSize)
            {
                eventsToProcess.assign(g_State.timeline.begin() + lastProcessed, g_State.timeline.end());
                g_State.processedCount.store(currentSize);
            }
        }

        for (const auto& gameEvent : eventsToProcess)
        {
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
        }

        std::this_thread::sleep_for(50ms);
    }

    Logger::LogAppend("=== Log Thread Stopped ===");
}