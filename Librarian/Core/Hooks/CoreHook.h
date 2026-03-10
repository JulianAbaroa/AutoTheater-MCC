#pragma once

#include <memory>

struct CoreAudioHook;
struct CoreDataHook;
struct CoreLifecycleHook;
struct CoreMemoryHook;
struct CoreInputHook;
struct CoreRenderHook;
struct CoreWindowHook;

struct CoreHook
{
	CoreHook();
	~CoreHook();

	std::unique_ptr<CoreAudioHook> Audio;
	std::unique_ptr<CoreDataHook> Data;
	std::unique_ptr<CoreInputHook> Input;
	std::unique_ptr<CoreLifecycleHook> Lifecycle;
	std::unique_ptr<CoreMemoryHook> Memory;
	std::unique_ptr<CoreRenderHook> Render;
	std::unique_ptr<CoreWindowHook> Window;
};

extern CoreHook* g_pHook;