#pragma once

#include "Core/Common/Types/AudioTypes.h"
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <map>

struct AudioState
{
public:
	bool IsRecording() const;
	void SetRecording(bool value);

	BYTE* GetBufferForInstance(void* instanece);
	void SetBufferForInstance(void* instance, BYTE* buffer);

	void* GetMasterInstance() const;
	void SetMasterInstance(void* newInstance);

	void RegisterAudioInstance(void* instance, WORD channels, DWORD samplesPerSec, WORD bytesPerFrame);
	AudioFormat GetAudioInstance(void* instance);
	std::map<void*, AudioFormat> GetAudioInstances();
	
	void Cleanup();

private:
	std::atomic<bool> m_IsRecording{ false };
	std::atomic<void*> m_MasterInstance{ nullptr };

	std::map<void*, AudioFormat> m_AudioInstances{};
	std::unordered_map<void*, BYTE*> m_InstanceBuffers{};
	mutable std::mutex m_Mutex;
};