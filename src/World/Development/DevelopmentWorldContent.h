#pragma once

#include "../../Reward/RewardTableType.h"
#include "../../Stats/CombatRatings.h"
#include "../../Entity/Monster/MonsterAggressionDefinition.h"
#include "../../Entity/Monster/MonsterRespawnDefinition.h"
#include "../Object/Resource/ResourceType.h"
#include "../Object/Station/StationType.h"

#include <optional>
#include <vector>

struct DevelopmentNpcSpawnDefinition
{
    int spawnX;
    int spawnY;
};

struct DevelopmentMonsterSpawnDefinition
{
    int spawnX;
    int spawnY;
    CombatRatings ratings;
    RewardTableType rewardTableType;
    std::optional<MonsterAggressionDefinition> aggressionDefinition;
    std::optional<MonsterRespawnDefinition> respawnDefinition;
};

struct DevelopmentResourcePlacementDefinition
{
    ResourceType resourceType;
    int x;
    int y;
};

struct DevelopmentStationPlacementDefinition
{
    StationType stationType;
    int x;
    int y;
};

namespace DevelopmentWorldContent
{
    constexpr int MapWidth = 100;
    constexpr int MapHeight = 100;

    const std::vector<DevelopmentNpcSpawnDefinition> &
    GetStarterNPCSpawns();

    const std::vector<DevelopmentMonsterSpawnDefinition> &
    GetStarterMonsterSpawns();

    const std::vector<DevelopmentResourcePlacementDefinition> &
    GetStarterResourcePlacements();

    const std::vector<DevelopmentStationPlacementDefinition> &
    GetStarterStationPlacements();
}
