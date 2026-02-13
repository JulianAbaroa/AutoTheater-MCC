#pragma once

#include "Core/Common/Types/TimelineTypes.h"
#include <functional>
#include <vector>
#include <atomic>
#include <mutex>

// TODO: Reverse Engineer the ReplayModule to create a C++ struct.

class TheaterState
{
public:
	// Getters
	bool IsTheaterMode() const;
	uintptr_t GetReplayModule() const;
	bool IsThirdPersonForced() const;
	uint8_t GetSpectatedPlayerIndex() const;
	uint8_t GetCameraMode() const;
	float* GetTimePtr() const;
	float* GetTimeScalePtr() const;

	// Setters
	void SetTheaterMode(bool theaterMode);
	void SetReplayModule(uintptr_t replayModule);
	void SetThirdPersonForced(bool attachToPOV);
	void SetSpectatedPlayerIndex(uint8_t playerIdx);
	void SetCameraMode(uint8_t cameraAttached);
	void SetTimePtr(float* pTime);
	void SetTimeScalePtr(float* pTimeScale);

	// PlayerList-related
	void ForEachPlayer(std::function<void(const PlayerInfo&)> callback);
	void SetPlayerList(const std::vector<PlayerInfo>& newList);
	std::vector<PlayerInfo> GetPlayerListCopy() const;
	void ResetPlayerList();

private:
	// Stores the player list of the last game event captured.
	std::vector<PlayerInfo> m_PlayerList{ 16 };
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_IsTheaterMode{ false };
	std::atomic<uintptr_t> m_pReplayModule{ 0 };
	std::atomic<bool> m_ThirdPersonForced{ false };
	std::atomic<uint8_t> m_SpectatedPlayerIndex{ 255 };
	std::atomic<uint8_t> m_CameraMode{ 0xFF };
	std::atomic<float*> m_pReplayTime{ nullptr };
	std::atomic<float*> m_pReplayTimeScale{ nullptr };
};