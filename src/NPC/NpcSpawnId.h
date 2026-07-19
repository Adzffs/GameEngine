#pragma once

enum class NpcSpawnId
{
    NONE,
    PASSIVE_DEVELOPMENT_SPAWN,
    AGGRESSIVE_DEVELOPMENT_SPAWN
};

constexpr bool IsValidNpcSpawnId(NpcSpawnId spawnId)
{
    switch (spawnId)
    {
    case NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN:
    case NpcSpawnId::AGGRESSIVE_DEVELOPMENT_SPAWN:
        return true;
    case NpcSpawnId::NONE:
        return false;
    }
    return false;
}
