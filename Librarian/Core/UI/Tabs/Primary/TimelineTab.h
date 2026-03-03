#pragma once

#include "Core/Common/Types/TimelineTypes.h"

class TimelineTab
{
public:
	void Draw();

private:
	void DrawTimelineControls(bool& autoScroll);
	void RenderPlayerCell(const std::vector<PlayerInfo>& players);
};