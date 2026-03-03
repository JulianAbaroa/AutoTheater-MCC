#pragma once

#include "Core/Common/Types/AudioTypes.h"
#include <atomic>
#include <mutex>
#include <map>

struct AudioState
{
public:
	void* GetMasterInstance() const;
	void SetMasterInstance(void* newInstance);
	void ResetMasterInstance();

	BYTE* GetLastBuffer() const;
	void SetLastBuffer(BYTE* pBuffer);

	bool IsRecording() const;
	void SetRecording(bool value);

	void RegisterActiveInstance(void* instance, WORD channels, DWORD samplesPerSec, WORD bytesPerFrame);
	AudioFormat GetActiveInstance(void* instance);
	std::map<void*, AudioFormat> GetActiveInstances();
	void ClearActiveInstances();
	
	void ResetForRecording();
	void Cleanup();

private:
	std::map<void*, AudioFormat> m_ActiveInstances{};
	mutable std::mutex m_Mutex;
	
	std::atomic<void*> m_MasterInstance{ nullptr };
	std::atomic<BYTE*> m_LastBuffer{ nullptr };
	std::atomic<bool> m_IsRecording{ false };
};