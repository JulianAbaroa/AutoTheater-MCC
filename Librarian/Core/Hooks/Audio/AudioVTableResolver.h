#pragma once

class AudioVTableResolver
{
public:
	void* GetAudioClientAddress(int index);
	void* GetRenderClientAddress(int index);
};