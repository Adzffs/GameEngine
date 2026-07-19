#pragma once

#include "NpcType.h"
#include "NpcSpawnId.h"
#include "../World/Position.h"

struct NpcSpawnDefinition
{
    NpcSpawnId spawnId = NpcSpawnId::NONE;
    NpcType npcType = NpcType::NONE;
    Position spawnPosition{0, 0};
    int wanderRadius = 0;
    int wanderIntervalTicks = 0;
    int maximumActiveCount = 0;
    bool respawns = false;
};
