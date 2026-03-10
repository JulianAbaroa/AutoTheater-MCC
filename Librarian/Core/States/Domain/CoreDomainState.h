#pragma once

#include <memory>

struct DirectorState;
struct EventRegistryState;
struct TheaterState;
struct TimelineState;

struct CoreDomainState
{
	CoreDomainState();
	~CoreDomainState();

	std::unique_ptr<DirectorState> Director;
	std::unique_ptr<EventRegistryState> EventRegistry;
	std::unique_ptr<TheaterState> Theater;
	std::unique_ptr<TimelineState> Timeline;
};