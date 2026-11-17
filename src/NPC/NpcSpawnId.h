#pragma once

enum class NpcSpawnId
{
    NONE,
    DEVELOPMENT_GUIDE_SPAWN,
    PASSIVE_DEVELOPMENT_SPAWN,
    AGGRESSIVE_DEVELOPMENT_SPAWN,
    DURABLE_DEVELOPMENT_SPAWN,
    ELITE_DEVELOPMENT_SPAWN
};

constexpr bool IsValidNpcSpawnId(NpcSpawnId spawnId)
{
    switch (spawnId)
    {
    case NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN:
    case NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN:
    case NpcSpawnId::AGGRESSIVE_DEVELOPMENT_SPAWN:
    case NpcSpawnId::DURABLE_DEVELOPMENT_SPAWN:
    case NpcSpawnId::ELITE_DEVELOPMENT_SPAWN:
        return true;
    case NpcSpawnId::NONE:
        return false;
    }
    return false;
}
