#include "pch.h"
#include "Utils/Logger.h"
#include "Core/States/Interface/InputState.h"

InputRequest InputState::GetNextRequest() const
{
	// We don't use a mutex here because the hkGetButtonState() is very sensitive.
	return m_NextRequest;
}

bool InputState::IsProcessing() const
{
	return m_IsProcessing.load();
}


void InputState::SetNextRequest(InputContext context, InputAction action)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_IsProcessing.store(true);
	m_NextRequest = { context, action };
}

void InputState::SetProcessing(bool processing)
{
	m_IsProcessing.store(processing);
}


void InputState::EnqueueRequest(const InputRequest& request, bool uniqueRequest)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (uniqueRequest && !m_Queue.empty())
	{
		if (m_Queue.back().Action == request.Action) return;
	}

	m_Queue.push(request);
}

bool InputState::DequeueRequest(InputRequest& outRequest)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Queue.empty()) return false;
	outRequest = m_Queue.front();
	m_Queue.pop();
	return true;
}

void InputState::Reset()
{
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		std::queue<InputRequest> empty;
		std::swap(m_Queue, empty);

		m_NextRequest = { InputContext::Unknown, InputAction::Unknown };
	}

	m_IsProcessing.store(false);
}