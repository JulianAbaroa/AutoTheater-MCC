#pragma once

#include "Core/Common/Types/DirectorTypes.h"
#include <vector>
#include <atomic>
#include <mutex>

class DirectorState
{
public:
	bool IsInitialized() const;
	bool IsSkipped() const;
	bool AreHooksReady() const;

	void SetInitialized(bool initialized);
	void SetSkipped(bool skipped);
	void SetHooksReady(bool hooksReady);
	
	std::vector<DirectorCommand> GetScriptCopy() const;
	void SetScript(std::vector<DirectorCommand> newScript);
	size_t GetScriptSize() const;
	void ClearScript();

	void Cleanup();

private:
	// Stores all the Director commands generated from the 'g_pState->Timeline.m_Events'.
	std::vector<DirectorCommand> m_Script{};
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_Initialized{ false };
	std::atomic<bool> m_Skipped{ false };
	std::atomic<bool> m_HooksReady{ false };
};