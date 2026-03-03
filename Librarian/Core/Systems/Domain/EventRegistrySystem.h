#pragma once

class EventRegistrySystem
{
public:
	EventRegistrySystem();

	void SaveEventRegistry();
	void LoadEventRegistry();

	void InitializeDefaultRegistry();

private:
	EventInfo BuildEvent(EventType type, int weight);
	EventClass ResolveEventClass(EventType type);
};