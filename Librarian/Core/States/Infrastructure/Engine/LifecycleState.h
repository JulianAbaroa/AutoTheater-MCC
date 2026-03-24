#pragma once

#include "Core/Common/Types/AppTypes.h"
#include <condition_variable>
#include <atomic>

class LifecycleState
{
public:
	bool IsRunning() const;
	HMODULE GetHandleModule() const;
	EngineStatus GetEngineStatus() const;
	Phase GetCurrentPhase() const;
	bool ShouldAutoUpdatePhase() const;
	std::mutex& GetMutex() const;
	std::condition_variable& GetCV() const;
	
	void SetRunning(bool value);
	void SetHandleModule(HMODULE value);
	void SetEngineStatus(EngineStatus value);
	void SetCurrentPhase(Phase value);
	void SetAutoUpdatePhase(bool value);

private:
	std::atomic<bool> m_IsRunning{ false };
	std::atomic<HMODULE> m_HandleModule{ nullptr };
	std::atomic<EngineStatus> m_EngineStatus{ EngineStatus::Waiting };
	std::atomic<Phase> m_CurrentPhase{ Phase::Default };
	std::atomic<bool> m_AutoUpdatePhase{ true };

	mutable std::condition_variable m_ShutdownCV{};
	mutable std::mutex m_Mutex{};
};