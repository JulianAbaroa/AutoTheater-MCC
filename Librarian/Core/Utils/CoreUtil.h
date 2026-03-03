#pragma once

#include "Core/Utils/DXUtil.h"
#include "Core/Utils/FormatUtil.h"
#include "Core/Utils/LogUtil.h"
#include "Core/Utils/ThreadUtil.h"

struct CoreUtil
{
	DXUtil DX;
	FormatUtil Format;
	LogUtil Log;
	ThreadUtil Thread;
};

extern CoreUtil* g_pUtil;