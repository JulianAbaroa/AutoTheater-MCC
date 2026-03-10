#pragma once

#include <memory>

class DirectorSystem;
class EventRegistrySystem;
class TheaterSystem;
class TimelineSystem;

struct CoreDomainSystem
{
	CoreDomainSystem();
	~CoreDomainSystem();

	std::unique_ptr<DirectorSystem> Director;
	std::unique_ptr<EventRegistrySystem> EventRegistry;
	std::unique_ptr<TheaterSystem> Theater;
	std::unique_ptr<TimelineSystem> Timeline;
};