#pragma once

#include "Core/Common/Types/AutoTheaterTypes.h"
#include "Core/Common/Types/UITypes.h"

class MainInterface
{
public:
	void Draw();

private:
	void HandleWindowReset();
	void DrawStatusBar();
	void DrawTabs();

	PhaseUI GetPhaseUI(AutoTheaterPhase phase);
};