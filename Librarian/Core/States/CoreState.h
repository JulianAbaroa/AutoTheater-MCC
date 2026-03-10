#pragma once

#include <memory>

struct CoreDomainState;
struct CoreInfrastructureState;
struct DebugState;

struct CoreState
{
	CoreState();
	~CoreState();

	std::unique_ptr<CoreDomainState> Domain;
	std::unique_ptr<CoreInfrastructureState> Infrastructure;
	std::unique_ptr<DebugState> Debug;
};

extern CoreState* g_pState;