#include "DevelopmentWorldContent.h"

namespace DevelopmentWorldContent
{
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
            {StationType::FURNACE, 14, 9}};

        return definitions;
    }
}
