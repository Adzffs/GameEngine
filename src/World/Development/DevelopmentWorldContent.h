#pragma once

#include "../Object/Resource/ResourceType.h"
#include "../Object/Station/StationType.h"
#include "../Tile/TileType.h"

#include <vector>

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

struct DevelopmentTerrainRegionDefinition
{
    TileType tileType;
    int minimumX;
    int minimumY;
    int maximumX;
    int maximumY;
};

namespace DevelopmentWorldContent
{
    constexpr int MapWidth = 100;
    constexpr int MapHeight = 100;
    inline constexpr int PlayerSpawnX = 0;
    inline constexpr int PlayerSpawnY = 0;

    const std::vector<DevelopmentResourcePlacementDefinition> &
    GetStarterResourcePlacements();

    const std::vector<DevelopmentStationPlacementDefinition> &
    GetStarterStationPlacements();

    const std::vector<DevelopmentTerrainRegionDefinition> &
    GetStarterTerrainRegions();
}
