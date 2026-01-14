#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Systems/Theater.h"
#include "Core/Systems/Director.h"
#include "Core/Systems/Timeline.h"
#include "Core/Threads/TheaterThread.h"
#include "Hooks/Lifecycle/GameEngineStart_Hook.h"

std::thread g_TheaterThread;
bool g_LogGameEvents = false;

std::string TheaterThread::EventTypeToString(EventType type) {
    switch (type) {
        // Server types
        case EventType::Join:				return "Join";
        case EventType::Rejoin:				return "Rejoin";
        case EventType::Quit:				return "Quit";
        case EventType::Booted:             return "Booted";
        case EventType::Banned:             return "Banned";
        case EventType::MinutesRemaining:   return "MinutesRemaining";

        // Gametypes
        case EventType::CaptureTheFlag:		return "CaptureTheFlag";

        // Match
        case EventType::TookLead:			return "TookLead";
        case EventType::TiedLead:			return "TiedLead";
        case EventType::GameOver:			return "GameOver";
        case EventType::Wins:				return "Wins";

        // Death types
        case EventType::Kill:				return "Kill";
        case EventType::Betrayal:			return "Betrayal";
        case EventType::Suicide:			return "Suicide";

        // Objective types
        case EventType::ObjectiveStolen:	return "ObjectiveStolen";
        case EventType::ObjectiveCaptured:	return "ObjectiveCaptured";
        case EventType::ObjectiveReseted:	return "ObjectiveReseted";
        case EventType::ObjectiveRecovered:	return "ObjectiveRecovered";
        case EventType::ObjectiveDropped:	return "ObjectiveDropped";

        // Medal types
        case EventType::Headshot:			return "Headshot";
        case EventType::Pummeled:			return "Pummeled";
        case EventType::Stuck:				return "Stuck";
        case EventType::Stopped:			return "Stopped";
        case EventType::KillingSpree:		return "KillingSpree";
        case EventType::Splattered:			return "Splattered";
        case EventType::BeatDown:			return "BeatDown";
        case EventType::Assassinated:		return "Assassinated";
        case EventType::KillingFrenzy:		return "KillingFrenzy";
        case EventType::RunningRiot:		return "RunningRiot";
        case EventType::Lasered:		    return "Lasered";
        case EventType::Sniped:		        return "Sniped";
        case EventType::Rampage:		    return "Rampage";

        // Special types
        case EventType::Struck:				return "Struck";
        case EventType::DoubleKill:			return "DoubleKill";
        case EventType::TripleKill:			return "TripleKill";
        case EventType::SwordSpree:			return "SwordSpree";
        case EventType::ReloadThis:			return "ReloadThis";
        case EventType::Revenge:			return "Revenge";
        case EventType::Killjoy:			return "Killjoy";
        case EventType::Avenger:			return "Avenger";
        case EventType::Pull:				return "Pull";
        case EventType::Headcase:			return "Headcase";
        case EventType::Assist:				return "Assist";
        case EventType::CloseCall:			return "CloseCall";
        case EventType::FirstStrike:		return "FirstStrike";
        case EventType::Firebird:		    return "Firebird";
        case EventType::Protector:		    return "Protector";
        case EventType::Overkill:		    return "Overkill";
        case EventType::HammerSpree:		return "HammerSpree";

        default:							return "Unknown";
    }
}

void TheaterThread::Run()
{
    Logger::LogAppend("=== Theater Thread Started ===");

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

    Logger::LogAppend("=== Theater Thread Stopped ===");
}