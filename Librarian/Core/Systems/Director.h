#pragma once

#include "Core/Systems/Timeline.h"
#include <atomic>
#include <mutex>

enum class CommandType { Cut, SetSpeed };

struct DirectorCommand
{
    float Timestamp;
    CommandType Type;

    uint8_t TargetPlayerIdx;
    std::string TargetPlayerName;

    float SpeedValue;
    std::string Reason;
};

struct ActionSegment
{
    std::vector<EventType> TotalEvents;
    std::string PlayerName;
    uint8_t PlayerID;
    int TotalScore;
    float StartTime;
    float EndTime;
};

extern std::vector<DirectorCommand> g_Script;
extern std::mutex g_ScriptMutex;

extern std::atomic<bool> g_DirectorInitialized;
extern size_t g_CurrentCommandIndex;

namespace Director
{
	void Initialize();
	void Update();
}