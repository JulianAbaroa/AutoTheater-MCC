#include "pch.h"
#include "Core/States/Domain/TheaterState.h"

bool TheaterState::IsTheaterMode() const
{
	return m_IsTheaterMode.load();
}

uintptr_t TheaterState::GetReplayModule() const
{
	return m_pReplayModule.load();
}

bool TheaterState::IsThirdPersonForced() const
{
	return m_ThirdPersonForced.load();
}

uint8_t TheaterState::GetSpectatedPlayerIndex() const
{
	return m_SpectatedPlayerIndex.load();
}

uint8_t TheaterState::GetCameraMode() const
{
	return m_CameraMode.load();
}

float* TheaterState::GetTimePtr() const
{
	return m_pReplayTime.load();
}

float* TheaterState::GetTimeScalePtr() const
{ 
	return m_pReplayTimeScale.load(); 
}


void TheaterState::SetTheaterMode(bool theaterMode)
{
	m_IsTheaterMode.store(theaterMode);
}

void TheaterState::SetReplayModule(uintptr_t replayModule)
{
	m_pReplayModule.store(replayModule);
}

void TheaterState::SetThirdPersonForced(bool attachToPOV)
{
	m_ThirdPersonForced.store(attachToPOV);
}

void TheaterState::SetSpectatedPlayerIndex(uint8_t playerIdx)
{
	m_SpectatedPlayerIndex.store(playerIdx);
}

void TheaterState::SetCameraMode(uint8_t cameraAttached)
{
	m_CameraMode.store(cameraAttached);
}

void TheaterState::SetTimePtr(float* pTime)
{
	m_pReplayTime.store(pTime);
}

void TheaterState::SetTimeScalePtr(float* pTimeScale) 
{ 
	m_pReplayTimeScale.store(pTimeScale); 
}


void TheaterState::ForEachPlayer(std::function<void(const PlayerInfo&)> callback)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	for (const auto& player : m_PlayerList)
	{
		callback(player);
	}
}

void TheaterState::SetPlayerList(const std::vector<PlayerInfo>& newList)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_PlayerList = newList;
}

std::vector<PlayerInfo> TheaterState::GetPlayerListCopy() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_PlayerList;
}

void TheaterState::ResetPlayerList()
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	for (auto& player : m_PlayerList)
	{
		player = PlayerInfo{};
	}
}