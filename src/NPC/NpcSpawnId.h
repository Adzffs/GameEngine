#pragma once

enum class NpcSpawnId
{
    NONE,
    DEVELOPMENT_GUIDE_SPAWN,
    PASSIVE_DEVELOPMENT_SPAWN,
    AGGRESSIVE_DEVELOPMENT_SPAWN,
    DURABLE_DEVELOPMENT_SPAWN,
    ELITE_DEVELOPMENT_SPAWN,
    TORREN_ASH_SPAWN,
    ELIA_REED_SPAWN,
    NESSA_THORNE_SPAWN,
    BRANN_DEEPWELL_SPAWN,
    SERA_FLINT_SPAWN
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
    case NpcSpawnId::TORREN_ASH_SPAWN:
    case NpcSpawnId::ELIA_REED_SPAWN:
    case NpcSpawnId::NESSA_THORNE_SPAWN:
    case NpcSpawnId::BRANN_DEEPWELL_SPAWN:
    case NpcSpawnId::SERA_FLINT_SPAWN:
        return true;
    case NpcSpawnId::NONE:
        return false;
    }
    return false;
}
