#pragma once

#include "Core/Common/Types/AutoTheaterTypes.h"
#include <condition_variable>
#include <atomic>

struct LifecycleState
{
public:
	bool IsRunning() const;
	HMODULE GetHandleModule() const;
	EngineStatus GetEngineStatus() const;
	AutoTheaterPhase GetCurrentPhase() const;
	bool ShouldAutoUpdatePhase() const;

	void SetRunning(bool value);
	void SetHandleModule(HMODULE value);
	void SetEngineStatus(EngineStatus value);
	void SetCurrentPhase(AutoTheaterPhase value);
	void SetAutoUpdatePhase(bool value);

	std::mutex& GetMutex() const;
	std::condition_variable& GetCV() const;

private:
	std::atomic<bool> m_IsRunning{ false };
	std::atomic<HMODULE> m_HandleModule{ nullptr };
	std::atomic<EngineStatus> m_EngineStatus{ EngineStatus::Awaiting };
	std::atomic<AutoTheaterPhase> m_CurrentPhase{ AutoTheaterPhase::Default };
	std::atomic<bool> m_AutoUpdatePhase{ true };

	mutable std::condition_variable m_ShutdownCV;
	mutable std::mutex m_Mutex;
};