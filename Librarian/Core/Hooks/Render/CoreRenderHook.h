#pragma once

#include <memory>

class ResizeBuffersHook;
class PresentHook;

struct CoreRenderHook
{
	CoreRenderHook();
	~CoreRenderHook();

	std::unique_ptr<ResizeBuffersHook> ResizeBuffers;
	std::unique_ptr<PresentHook> Present;
};