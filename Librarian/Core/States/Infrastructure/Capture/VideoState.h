#pragma once

#include <atomic>

struct VideoState
{
public:
    bool IsRecording() const;
    void SetRecording(bool value);

    void Cleanup();

private:
    std::atomic<bool> m_IsRecording{ false };
};