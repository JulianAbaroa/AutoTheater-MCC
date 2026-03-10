#include "pch.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Director/DirectorSystem.h"
#include "Core/Systems/Domain/Director/EventRegistrySystem.h"
#include "Core/Systems/Domain/Theater/TheaterSystem.h"
#include "Core/Systems/Domain/Timeline/TimelineSystem.h"

CoreDomainSystem::CoreDomainSystem()
{
	Director = std::make_unique<DirectorSystem>();
	EventRegistry = std::make_unique<EventRegistrySystem>();
	Theater = std::make_unique<TheaterSystem>();
	Timeline = std::make_unique<TimelineSystem>();
}

CoreDomainSystem::~CoreDomainSystem() = default;