#pragma once

#include <memory>

class AudioClientInitializeHook;
class ReleaseBufferHook;
class GetServiceHook;
class GetBufferHook;

struct CoreAudioHook
{
	CoreAudioHook();
	~CoreAudioHook();

	std::unique_ptr<AudioClientInitializeHook> AudioClientInitialize;
	std::unique_ptr<ReleaseBufferHook> ReleaseBuffer;
	std::unique_ptr<GetServiceHook> GetService;
	std::unique_ptr<GetBufferHook> GetBuffer;
};