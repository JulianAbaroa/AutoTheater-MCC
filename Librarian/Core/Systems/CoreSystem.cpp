#include "pch.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

CoreSystem::CoreSystem()
{
	Domain = std::make_unique<CoreDomainSystem>();
	Infrastructure = std::make_unique<CoreInfrastructureSystem>();
	Debug = std::make_unique<DebugSystem>();
}

CoreSystem::~CoreSystem() = default;