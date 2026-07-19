#pragma once

#include "NpcType.h"
#include "../World/Position.h"

struct NpcSpawnDefinition
{
    NpcType npcType = NpcType::NONE;
    Position spawnPosition{0, 0};
    int wanderRadius = 0;
    bool respawns = false;
};
