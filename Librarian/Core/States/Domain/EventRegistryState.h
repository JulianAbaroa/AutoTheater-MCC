#pragma once

#include "Core/Common/Types/TimelineTypes.h"
#include "Core/Common/EventRegistry.h"
#include <unordered_map>
#include <functional>
#include <mutex>

struct EventRegistryState
{
public:
	bool IsEventRegistered(const std::wstring& templateStr) const;
	EventType GetEventType(const std::wstring& templateStr) const;

	void SetEventRegistry(std::unordered_map<std::wstring, EventInfo> newRegistry);

	std::unordered_map<std::wstring, EventInfo> GetEventRegistryCopy() const;
	void ForEachEvent(std::function<void(const std::wstring&, const EventInfo&)> callback) const;

	void UpdateWeightsByType(EventType type, int newWeight);

private:
	std::unordered_map<std::wstring, EventInfo> m_EventRegistry = g_EventRegistry;
	mutable std::mutex m_Mutex;
};