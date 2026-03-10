#pragma once

#include <memory>

struct CoreDomainSystem;
struct CoreInfrastructureSystem;
class DebugSystem;

struct CoreSystem
{
	CoreSystem();
	~CoreSystem();

	std::unique_ptr<CoreDomainSystem> Domain;
	std::unique_ptr<CoreInfrastructureSystem> Infrastructure;
	std::unique_ptr<DebugSystem> Debug;
};

extern CoreSystem* g_pSystem;