#include "pch.h"
#include "Utils/Logger.h"
#include "Core/States/Domain/DirectorState.h"

bool DirectorState::IsInitialized() const
{ 
	return m_Initialized.load(); 
}

bool DirectorState::AreHooksReady() const
{
	return m_HooksReady.load();
}


void DirectorState::SetInitialized(bool initialized) 
{ 
	m_Initialized.store(initialized); 
}

void DirectorState::SetHooksReady(bool hooksReady) 
{ 
	m_HooksReady.store(hooksReady); 
}


void DirectorState::SetScript(std::vector<DirectorCommand> newScript)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Script = std::move(newScript);
}

std::vector<DirectorCommand> DirectorState::GetScriptCopy() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Script;
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