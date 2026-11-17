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
             Position{18, 2}, 0, 0, 1, true},
            {NpcSpawnId::DURABLE_DEVELOPMENT_SPAWN, NpcType::DURABLE_DEVELOPMENT_MONSTER,
             Position{16, 11}, 1, 7, 1, true},
            {NpcSpawnId::ELITE_DEVELOPMENT_SPAWN, NpcType::ELITE_DEVELOPMENT_MONSTER,
             Position{22, 3}, 0, 0, 1, true},
            {NpcSpawnId::TORREN_ASH_SPAWN, NpcType::TORREN_ASH,
             Position{16, 10}, 0, 0, 1, false}};
        return spawns;
    }

    const std::vector<NpcSpawnDefinition> &GetStarterMonsterSpawns()
    {
        static const std::vector<NpcSpawnDefinition> spawns{
            {NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN, NpcType::PASSIVE_DEVELOPMENT_MONSTER,
             Position{6, 1}, 2, 5, 1, true},
            {NpcSpawnId::AGGRESSIVE_DEVELOPMENT_SPAWN, NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER,
             Position{18, 2}, 0, 0, 1, true},
            {NpcSpawnId::DURABLE_DEVELOPMENT_SPAWN, NpcType::DURABLE_DEVELOPMENT_MONSTER,
             Position{16, 11}, 1, 7, 1, true},
            {NpcSpawnId::ELITE_DEVELOPMENT_SPAWN, NpcType::ELITE_DEVELOPMENT_MONSTER,
             Position{22, 3}, 0, 0, 1, true}};
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
