#pragma once

#include <memory>

// Director
class DirectorState;
class EventRegistryState;

// Theater
class TheaterState;

// Timeline
class TimelineState;

// Main container for the application's domain states.
struct CoreDomainState
{
	CoreDomainState();
	~CoreDomainState();

	std::unique_ptr<DirectorState> Director;
	std::unique_ptr<EventRegistryState> EventRegistry;
	std::unique_ptr<TheaterState> Theater;
	std::unique_ptr<TimelineState> Timeline;
};