#include "RecipeDatabase.h"

#include <stdexcept>

const std::vector<RecipeType> &
RecipeDatabase::GetAllRecipeTypes()
{
    static const std::vector<RecipeType> recipeTypes{
        RecipeType::BRONZE_BAR,
        RecipeType::IRON_BAR,
        RecipeType::STEEL_BAR};

    return recipeTypes;
}

const RecipeDefinition &
RecipeDatabase::Get(
    RecipeType recipeType)
{
    static const RecipeDefinition bronzeBarRecipe{
        RecipeType::BRONZE_BAR,
        "Smelt Bronze bar",
        StationType::FURNACE,
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
        StationType::FURNACE,
        SkillType::SMITHING,
        10,
        13,
        3,
        {
            {ItemType::IRON_ORE, 1},
        },
        ItemType::IRON_BAR,
        1};

    static const RecipeDefinition steelBarRecipe{
        RecipeType::STEEL_BAR,
        "Smelt Steel bar",
        StationType::FURNACE,
        SkillType::SMITHING,
        20,
        20,
        4,
        {
            {ItemType::IRON_ORE, 1},
            {ItemType::COAL, 2},
        },
        ItemType::STEEL_BAR,
        1};

    switch (recipeType)
    {
    case RecipeType::BRONZE_BAR:
        return bronzeBarRecipe;

    case RecipeType::IRON_BAR:
        return ironBarRecipe;

    case RecipeType::STEEL_BAR:
        return steelBarRecipe;

    case RecipeType::NONE:
    default:
        throw std::invalid_argument(
            "Unknown recipe type");
    }
}
