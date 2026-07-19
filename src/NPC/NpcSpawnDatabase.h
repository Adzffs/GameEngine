#pragma once

#include "NpcSpawnDefinition.h"

#include <vector>

namespace NpcSpawnDatabase
{
    const std::vector<NpcSpawnDefinition> &GetStarterMonsterSpawns();
    const NpcSpawnDefinition *TryGet(NpcSpawnId spawnId);
}
