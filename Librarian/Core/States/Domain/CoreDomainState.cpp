#include "pch.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Director/DirectorState.h"
#include "Core/States/Domain/Director/EventRegistryState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Domain/Timeline/TimelineState.h"

CoreDomainState::CoreDomainState()
{
	Director = std::make_unique<DirectorState>();
	EventRegistry = std::make_unique<EventRegistryState>();
	Theater = std::make_unique<TheaterState>();
	Timeline = std::make_unique<TimelineState>();
}

CoreDomainState::~CoreDomainState() = default;