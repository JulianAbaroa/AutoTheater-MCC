#pragma once

#include "Core/Common/Types/VideoTypes.h"
#include <cstdint>
#include <vector>
#include <deque>

class VideoSystem
{
public:
	void StartRecording();
	void StopRecording();

	void PushFrame(const uint8_t* pData, UINT width, UINT height, UINT rowPitch, double engineTime);
	
	std::deque<FrameData> ExtractQueue();
	void ClearQueue();
	
	void PreallocatePool(UINT width, UINT height);
	void ReturnBuffer(std::vector<uint8_t>&& buffer);
	std::vector<uint8_t> GetFreeBuffer();

	void Cleanup();
};