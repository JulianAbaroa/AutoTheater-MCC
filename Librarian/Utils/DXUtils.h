#pragma once

#define PRESENT_VMT_INDEX 8
#define RESIZE_BUFFERS_VMT_INDEX 13

struct DX11Addresses
{
	void* Present;
	void* ResizeBuffers;
};

namespace DXUtils
{
	DX11Addresses GetVtableAddresses();
}