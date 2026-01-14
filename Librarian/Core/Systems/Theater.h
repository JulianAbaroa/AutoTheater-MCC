#pragma once

#include "Core/Systems/RawTypes.h"
#include <string>
#include <vector>

struct PlayerInfo {
    RawPlayer RawPlayer;
    std::vector<RawWeapon> Weapons;

    std::string Name;
    std::string Tag;
    bool IsVictim;
    uint8_t Id;
};

extern std::vector<PlayerInfo> g_PlayerList;
extern float* g_pReplayTimeScale;

namespace Theater
{
    void SetReplaySpeed(float speed);
    void RebuildPlayerListFromMemory();

    bool TryGetFollowedPlayerIdx(uint64_t pReplayModule);
    bool TryGetPlayerName(uint8_t slotID, wchar_t* outName, size_t maxChars);
}