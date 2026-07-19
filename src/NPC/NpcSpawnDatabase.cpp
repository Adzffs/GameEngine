#include "NpcSpawnDatabase.h"

namespace NpcSpawnDatabase
{
    const std::vector<NpcSpawnDefinition> &GetStarterSpawns()
    {
        static const std::vector<NpcSpawnDefinition> spawns{
            {NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN, NpcType::DEVELOPMENT_GUIDE,
             Position{3, 3}, 0, 0, 1, false},
            {NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN, NpcType::PASSIVE_DEVELOPMENT_MONSTER,
             Position{6, 1}, 2, 5, 1, true},
            {NpcSpawnId::AGGRESSIVE_DEVELOPMENT_SPAWN, NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER,
             Position{18, 2}, 0, 0, 1, true}};
        return spawns;
    }

    const std::vector<NpcSpawnDefinition> &GetStarterMonsterSpawns()
    {
        static const std::vector<NpcSpawnDefinition> spawns{
            {NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN, NpcType::PASSIVE_DEVELOPMENT_MONSTER,
             Position{6, 1}, 2, 5, 1, true},
            {NpcSpawnId::AGGRESSIVE_DEVELOPMENT_SPAWN, NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER,
             Position{18, 2}, 0, 0, 1, true}};
        return spawns;
    }

    const NpcSpawnDefinition *TryGet(NpcSpawnId spawnId)
    {
        for (const NpcSpawnDefinition &spawn : GetStarterSpawns())
        {
            if (spawn.spawnId == spawnId)
                return &spawn;
        }
        return nullptr;
    }
}
