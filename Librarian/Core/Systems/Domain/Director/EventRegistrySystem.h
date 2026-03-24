#pragma once

class EventRegistrySystem
{
public:
	EventRegistrySystem();
	void InitializeDefaultRegistry();

	void SaveEventRegistry();
	void LoadEventRegistry();

private:
	EventInfo BuildEvent(EventType type, int weight);
	EventClass ResolveEventClass(EventType type);
};