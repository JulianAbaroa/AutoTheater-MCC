#pragma once

#include "Core/Common/Types/DirectorTypes.h"
#include <vector>
#include <atomic>
#include <mutex>

struct DirectorState
{
public:
	bool IsInitialized() const;
	bool AreHooksReady() const;

	void SetInitialized(bool initialized);
	void SetHooksReady(bool hooksReady);
	
	void SetScript(std::vector<DirectorCommand> newScript);
	std::vector<DirectorCommand> GetScriptCopy() const;
	size_t GetScriptSize() const;
	void ClearScript();

private:
	// Stores all the Director commands generated from the 'g_pState->Timeline.m_Events'.
	std::vector<DirectorCommand> m_Script{};
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_Initialized{ false };
	std::atomic<bool> m_HooksReady{ false };
};