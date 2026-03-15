#include "pch.h"
#include "Core/States/Infrastructure/Capture/MuxerState.h"

void MuxerState::SetReady(bool v) { m_Ready.store(v); }
bool MuxerState::IsReady() const { return m_Ready.load(); }

void MuxerState::SetStartTime(double t) { m_StartTime.store(t); }
double MuxerState::GetStartTime() const { return m_StartTime.load(); }