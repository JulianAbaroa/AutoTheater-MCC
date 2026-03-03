#include "pch.h"
#include "Core/States/Infrastructure/VideoState.h"

bool VideoState::IsRecording() const { return m_IsRecording.load(); }
void VideoState::SetRecording(bool value) { m_IsRecording.store(value); }