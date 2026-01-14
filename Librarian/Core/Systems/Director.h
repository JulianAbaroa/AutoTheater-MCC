#pragma once

#include "Core/Systems/Timeline.h"
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
    float TotalScore;
    float StartTime;
    float EndTime;
};

extern std::vector<DirectorCommand> g_Script;
extern std::mutex g_ScriptMutex;

extern bool g_DirectorInitialized;
extern int g_CurrentCommandIndex;

namespace Director
{
	void Initialize();
	void Update();
}