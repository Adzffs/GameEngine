#include "DevelopmentWorldContent.h"

namespace DevelopmentWorldContent
{
    const std::vector<DevelopmentTerrainRegionDefinition> &
    GetStarterTerrainRegions()
    {
        static const std::vector<DevelopmentTerrainRegionDefinition> definitions{
            // The east-west road ties every early service together.
            {TileType::DIRT, 0, 2, 24, 3},
            // A stone village square makes the guide and bank legible landmarks.
            {TileType::STONE, 1, 1, 4, 4},
            {TileType::STONE, 19, 1, 22, 4},
            // Woodland loop with short spurs to each tree tier.
            {TileType::DIRT, 3, 4, 12, 4},
            {TileType::DIRT, 3, 4, 4, 7},
            {TileType::DIRT, 6, 4, 7, 6},
            {TileType::DIRT, 9, 4, 10, 6},
            {TileType::DIRT, 12, 3, 13, 6},
            // The mine and workshop use stone floors and a shared material road.
            {TileType::STONE, 3, 8, 12, 12},
            {TileType::DIRT, 3, 6, 4, 8},
            {TileType::STONE, 13, 7, 19, 12},
            {TileType::DIRT, 12, 5, 13, 8},
            {TileType::DIRT, 19, 3, 20, 8},
            // Training clearings are visually separate from the safe square.
            {TileType::DIRT, 5, 0, 9, 2},
            {TileType::DIRT, 16, 0, 19, 2}};
        return definitions;
    }

    const std::vector<DevelopmentResourcePlacementDefinition> &
    GetStarterResourcePlacements()
    {
        static const std::vector<DevelopmentResourcePlacementDefinition> definitions{
            {ResourceType::NORMAL_TREE, 5, 5},
            {ResourceType::OAK_TREE, 8, 5},
            {ResourceType::WILLOW_TREE, 11, 5},
            {ResourceType::COPPER_ROCK, 5, 9},
            {ResourceType::TIN_ROCK, 8, 9},
            {ResourceType::IRON_ROCK, 11, 9},
            {ResourceType::COAL_ROCK, 14, 12}};

        return definitions;
    }

    const std::vector<DevelopmentStationPlacementDefinition> &
    GetStarterStationPlacements()
    {
        static const std::vector<DevelopmentStationPlacementDefinition> definitions{
            {StationType::FURNACE, 14, 9},
            {StationType::CRAFTING_TABLE, 17, 9},
            {StationType::BANK, 20, 3}};

        return definitions;
    }
}
