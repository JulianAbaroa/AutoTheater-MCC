#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Interface/DebugState.h"

CoreState::CoreState()
{
	Domain = std::make_unique<CoreDomainState>();
	Infrastructure = std::make_unique<CoreInfrastructureState>();
	Debug = std::make_unique<DebugState>();
}

CoreState::~CoreState() = default;