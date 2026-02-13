#pragma once

#include "Core/Common/Types/TimelineTypes.h"
#include <cstdint>
#include <atomic>

class TheaterSystem
{
public: 
    void Update();

    void SetReplaySpeed(float speed);
    void RefreshPlayerList();

    bool TryGetSpectatedPlayerIndex(uint64_t pReplayModule);
    bool TryGetPlayerName(uint8_t slotID, wchar_t* outName, size_t maxChars);

    void UpdateRealTimeScale();
    float GetRealTimeScale() const;

private:
    std::atomic<float> m_RealTimeScale{ 1.0f };
    std::atomic<double> m_AnchorSystemTime{ 0.0f };
    std::atomic<float> m_AnchorReplayTime{ 0.0f };

    void LogTables(uintptr_t playerTable, uintptr_t objectTable);
    bool RawReadSinglePlayer(uintptr_t playerTable, uintptr_t objectTable, uint8_t index, PlayerInfo& outInfo);

    double GetAnchorSystemTime() const;
    float GetAnchorReplayTime() const;

    void SetRealTimeScale(float realTimeScale);
    void SetAnchorSystemTime(double anchorSystemTime);
    void SetAnchorReplayTime(float anchorReplayTime);
};