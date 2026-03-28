#include "pch.h"
#include "Core/States/Domain/Director/DirectorState.h"

bool DirectorState::IsInitialized() const { return m_Initialized.load(); }
bool DirectorState::IsSkipped() const { return m_Skipped.load(); }
bool DirectorState::AreHooksReady() const { return m_HooksReady.load(); }

void DirectorState::SetInitialized(bool initialized) { m_Initialized.store(initialized); }
void DirectorState::SetSkipped(bool skipped) { m_Skipped.store(skipped); }
void DirectorState::SetHooksReady(bool hooksReady) { m_HooksReady.store(hooksReady); }

std::vector<DirectorCommand> DirectorState::GetScriptCopy() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Script;
}

void DirectorState::SetScript(std::vector<DirectorCommand> newScript)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Script = std::move(newScript);
}

size_t DirectorState::GetScriptSize() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Script.size();
}

void DirectorState::ClearScript()
{
	std::lock_guard lock(m_Mutex);
	m_Script.clear();
}


void DirectorState::Cleanup()
{
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Script.clear();
	}

	m_Initialized.store(false);
	m_Skipped.store(false);
	m_HooksReady.store(false);
}