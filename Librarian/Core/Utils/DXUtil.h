#pragma once

#include "Core/Common/Types/DX11Types.h"

#define PRESENT_VMT_INDEX 8
#define RESIZE_BUFFERS_VMT_INDEX 13

class DXUtil
{
public:
	DX11Addresses GetVtableAddresses();
};