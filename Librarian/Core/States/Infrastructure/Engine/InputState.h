#pragma once

#include "Core/Common/Types/InputTypes.h"
#include <atomic>
#include <queue>
#include <mutex>

struct InputState
{
public:
	InputRequest GetNextRequest() const;
	bool IsProcessing() const;

	void SetNextRequest(InputContext context, InputAction action);
	void SetProcessing(bool processing);

	void EnqueueRequest(const InputRequest& request, bool uniqueRequest = false);
	bool DequeueRequest(InputRequest& outRequest);
	void Reset();

private:
	std::queue<InputRequest> m_Queue{};
	InputRequest m_NextRequest{ InputContext::Unknown, InputAction::Unknown };
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_IsProcessing{ false };
};