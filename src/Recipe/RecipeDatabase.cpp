#include "RecipeDatabase.h"

#include <stdexcept>

const RecipeDefinition &
RecipeDatabase::Get(
    RecipeType recipeType)
{
    static const RecipeDefinition bronzeBarRecipe{
        RecipeType::BRONZE_BAR,
        "Smelt Bronze bar",
        SkillType::SMITHING,
        1,
        6,
        3,
        {
            {ItemType::COPPER_ORE, 1},
            {ItemType::TIN_ORE, 1},
        },
        ItemType::BRONZE_BAR,
        1};

    static const RecipeDefinition ironBarRecipe{
        RecipeType::IRON_BAR,
        "Smelt Iron bar",
        SkillType::SMITHING,
        10,
        13,
        3,
        {
            {ItemType::IRON_ORE, 1},
        },
        ItemType::IRON_BAR,
        1};

    switch (recipeType)
    {
    case RecipeType::BRONZE_BAR:
        return bronzeBarRecipe;

    case RecipeType::IRON_BAR:
        return ironBarRecipe;

    case RecipeType::NONE:
    default:
        throw std::invalid_argument(
            "Unknown recipe type");
    }
}
