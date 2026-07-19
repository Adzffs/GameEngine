#include "NpcSpawnDatabase.h"

namespace NpcSpawnDatabase
{
    const std::vector<NpcSpawnDefinition> &GetStarterMonsterSpawns()
    {
        static const std::vector<NpcSpawnDefinition> spawns{
            {NpcType::PASSIVE_DEVELOPMENT_MONSTER, Position{6, 1}, 0, true},
            {NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER, Position{18, 2}, 0, true}};
        return spawns;
    }
}
