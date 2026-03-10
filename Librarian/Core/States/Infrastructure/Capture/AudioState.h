#pragma once

#include "Core/Common/Types/AudioTypes.h"
#include <atomic>
#include <mutex>
#include <map>

struct AudioState
{
public:
	bool IsRecording() const;
	BYTE* GetLastBuffer() const;
	
	void SetRecording(bool value);
	void SetLastBuffer(BYTE* pBuffer);

	void* GetMasterInstance() const;
	void SetMasterInstance(void* newInstance);

	void RegisterAudioInstance(void* instance, WORD channels, DWORD samplesPerSec, WORD bytesPerFrame);
	AudioFormat GetAudioInstance(void* instance);
	std::map<void*, AudioFormat> GetAudioInstances();
	
	void Cleanup();

private:
	std::atomic<bool> m_IsRecording{ false };
	std::atomic<BYTE*> m_LastBuffer{ nullptr };
	std::atomic<void*> m_MasterInstance{ nullptr };

	std::map<void*, AudioFormat> m_AudioInstances{};
	mutable std::mutex m_Mutex;
};