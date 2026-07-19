#pragma once

#include "../Object/Resource/ResourceType.h"
#include "../Object/Station/StationType.h"

#include <vector>

struct DevelopmentNpcSpawnDefinition
{
    int spawnX;
    int spawnY;
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

    const std::vector<DevelopmentResourcePlacementDefinition> &
    GetStarterResourcePlacements();

    const std::vector<DevelopmentStationPlacementDefinition> &
    GetStarterStationPlacements();
}
