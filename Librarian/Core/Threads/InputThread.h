#pragma once

#include <thread>

extern std::thread g_InputThread;

namespace InputThread
{
	void Run();

    void ToggleHUD();
    void TogglePlaybackHUD();

    void ToggleCameraMode();
    void ToggleFreecam();

    void NextPlayer();
    void PrevPlayer();

    void JumpForward();
    void JumpBack();

    void ResetCamera();
}