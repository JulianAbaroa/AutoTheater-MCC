#pragma once

#include "Core/Common/Types/TimelineTypes.h"
#include "Core/Common/Types/TheaterTypes.h"
#include <functional>
#include <optional>
#include <vector>
#include <atomic>
#include <mutex>

class TheaterState
{
public:
	bool IsTheaterMode() const;
	void SetTheaterMode(bool theaterMode);

	POVMode GetPOVMode() const;
	UIMode GetUIMode() const;
	CameraMode GetCameraMode() const;

	void SetPOVMode(POVMode value);
	void SetUIMode(UIMode value);
	void SetCameraMode(CameraMode value);

	float* GetTimePtr() const;
	float* GetTimeScalePtr() const;
	void SetTimePtr(float* pTime);
	void SetTimeScalePtr(float* pTimeScale);

	ReplayModule GetModuleSnapshot() const;
	void UpdateReplayModule(ReplayModule* pModule);

	std::optional<PlayerInfo> GetPlayerBySlot(uint8_t slotIndex) const;
	void SetPlayerList(const std::vector<PlayerInfo>& newList);
	void ForEachPlayer(std::function<void(const PlayerInfo&)> callback);
	void ResetPlayerList();

	void Cleanup();

private:
	// Stores the player list of the last game event captured.
	std::vector<PlayerInfo> m_PlayerList{ 16 };
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_IsTheaterMode{ false };

	// UI defined values.
	POVMode m_POVMode{ POVMode::Unselected };
	UIMode m_UIMode{ UIMode::Unselected };
	CameraMode m_CameraMode{ CameraMode::Unselected };

	std::atomic<float*> m_pReplayTime{ nullptr };
	std::atomic<float*> m_pReplayTimeScale{ nullptr };

	ReplayModule m_CachedReplayModule{};
	mutable std::mutex m_ReplayMutex;
};