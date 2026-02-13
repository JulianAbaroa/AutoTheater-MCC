#include "pch.h"
#include "Core/States/Domain/EventRegistryState.h"

bool EventRegistryState::IsEventRegistered(const std::wstring& templateStr) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_EventRegistry.find(templateStr) != m_EventRegistry.end();
}

EventType EventRegistryState::GetEventType(const std::wstring& templateStr) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	auto it = m_EventRegistry.find(templateStr);
	if (it != m_EventRegistry.end())
	{
		return it->second.Type;
	}

	return EventType::Unknown;
}


void EventRegistryState::SetEventRegistry(std::unordered_map<std::wstring, EventInfo> newRegistry)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_EventRegistry = std::move(newRegistry);
}


std::unordered_map<std::wstring, EventInfo> EventRegistryState::GetEventRegistryCopy() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_EventRegistry;
}

void EventRegistryState::ForEachEvent(std::function<void(const std::wstring&, const EventInfo&)> callback) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	for (const auto& [name, info] : m_EventRegistry)
	{
		callback(name, info);
	}
}


void EventRegistryState::UpdateWeightsByType(EventType type, int newWeight)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	for (auto& [name, info] : m_EventRegistry)
	{
		if (info.Type == type)
		{
			info.Weight = newWeight;
		}
	}
}