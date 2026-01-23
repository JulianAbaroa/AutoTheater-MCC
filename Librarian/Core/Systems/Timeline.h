#pragma once

#include "Core/Common/Types.h"

namespace Timeline
{
	void AddGameEvent(
		float timestamp, 
		std::wstring& templateStr,
		EventData* eventData
	);
};