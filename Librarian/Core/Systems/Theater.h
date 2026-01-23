#pragma once

#include <cstdint>

namespace Theater
{
    void SetReplaySpeed(float speed);
    void RebuildPlayerListFromMemory();

    bool TryGetFollowedPlayerIdx(uint64_t pReplayModule);
    bool TryGetPlayerName(uint8_t slotID, wchar_t* outName, size_t maxChars);
}