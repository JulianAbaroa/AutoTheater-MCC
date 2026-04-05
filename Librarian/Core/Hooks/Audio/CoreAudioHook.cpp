#include "pch.h"
#include "Core/Hooks/Audio/CoreAudioHook.h"
#include "Core/Hooks/Audio/AudioClientInitializeHook.h"
#include "Core/Hooks/Audio/ReleaseBufferHook.h"
#include "Core/Hooks/Audio/GetServiceHook.h"
#include "Core/Hooks/Audio/GetBufferHook.h"
#include "Core/Hooks/Audio/AudioVTableResolver.h"

CoreAudioHook::CoreAudioHook()
{
    AudioClientInitialize = std::make_unique<AudioClientInitializeHook>();
    ReleaseBuffer = std::make_unique<ReleaseBufferHook>();
    GetService = std::make_unique<GetServiceHook>();
    GetBuffer = std::make_unique<GetBufferHook>();
	Resolver = std::make_unique<AudioVTableResolver>();
}

CoreAudioHook::~CoreAudioHook() = default;