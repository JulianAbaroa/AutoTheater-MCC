#include "pch.h"
#include "Core/States/Domain/Theater/TheaterState.h"

bool TheaterState::IsTheaterMode() const { return m_IsTheaterMode.load(); }
float* TheaterState::GetTimePtr() const { return m_pReplayTime.load(); }
float* TheaterState::GetTimeScalePtr() const { return m_pReplayTimeScale.load(); }

void TheaterState::SetTheaterMode(bool theaterMode) { m_IsTheaterMode.store(theaterMode); }
void TheaterState::SetTimePtr(float* pTime) { m_pReplayTime.store(pTime); }
void TheaterState::SetTimeScalePtr(float* pTimeScale) { m_pReplayTimeScale.store(pTimeScale); }

POVMode TheaterState::GetPOVMode() const { return m_POVMode; }
UIMode TheaterState::GetUIMode() const { return m_UIMode; }
CameraMode TheaterState::GetCameraMode() const { return m_CameraMode; }

void TheaterState::SetPOVMode(POVMode value) { m_POVMode = value; }
void TheaterState::SetUIMode(UIMode value) { m_UIMode = value; }
void TheaterState::SetCameraMode(CameraMode value) { m_CameraMode = value; }

void TheaterState::UpdateReplayModule(ReplayModule* pModule)
{
	if (pModule != nullptr)
	{
		std::lock_guard<std::mutex> lock(m_ReplayMutex);
		m_CachedReplayModule = *pModule;
	}
}

ReplayModule TheaterState::GetModuleSnapshot() const
{
	std::lock_guard<std::mutex> lock(m_ReplayMutex);
	return m_CachedReplayModule;
}


std::optional<PlayerInfo> TheaterState::GetPlayerBySlot(uint8_t slotIndex) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	if (slotIndex < m_PlayerList.size())
	{
		return m_PlayerList[slotIndex];
	}

	return std::nullopt;
}

void TheaterState::SetPlayerList(const std::vector<PlayerInfo>& newList)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_PlayerList = newList;
}

void TheaterState::ForEachPlayer(std::function<void(const PlayerInfo&)> callback)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	for (const auto& player : m_PlayerList)
	{
		callback(player);
	}
}

void TheaterState::ResetPlayerList()
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	for (auto& player : m_PlayerList)
	{
		player = PlayerInfo{};
	}
}


void TheaterState::Cleanup()
{
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_PlayerList = {};
	}

	m_pReplayTime.store(nullptr);
	m_pReplayTimeScale.store(nullptr);
}