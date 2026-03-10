#include "pch.h"
#include "Core/Hooks/Render/CoreRenderHook.h"
#include "Core/Hooks/Render/ResizeBuffersHook.h"
#include "Core/Hooks/Render/PresentHook.h"

CoreRenderHook::CoreRenderHook()
{
	ResizeBuffers = std::make_unique<ResizeBuffersHook>();
	Present = std::make_unique<PresentHook>();
}

CoreRenderHook::~CoreRenderHook() = default;