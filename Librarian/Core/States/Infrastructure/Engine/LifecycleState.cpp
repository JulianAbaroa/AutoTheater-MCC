#include "pch.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"

bool LifecycleState::IsRunning() const { return m_IsRunning.load(); }
HMODULE LifecycleState::GetHandleModule() const { return m_HandleModule.load(); }
EngineStatus LifecycleState::GetEngineStatus() const { return m_EngineStatus.load(); }
Phase LifecycleState::GetCurrentPhase() const { return m_CurrentPhase.load(); }
bool LifecycleState::ShouldAutoUpdatePhase() const { return m_AutoUpdatePhase.load(); }
std::mutex& LifecycleState::GetMutex() const { return m_Mutex; }
std::condition_variable& LifecycleState::GetCV() const { return m_ShutdownCV; }

void LifecycleState::SetRunning(bool value) { m_IsRunning.store(value); }
void LifecycleState::SetHandleModule(HMODULE value) { m_HandleModule.store(value); }
void LifecycleState::SetEngineStatus(EngineStatus value) { m_EngineStatus.store(value); }
void LifecycleState::SetCurrentPhase(Phase value) { m_CurrentPhase.store(value); }
void LifecycleState::SetAutoUpdatePhase(bool value) { m_AutoUpdatePhase.store(value); }