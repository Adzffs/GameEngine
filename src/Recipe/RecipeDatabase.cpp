#include "RecipeDatabase.h"

#include <stdexcept>

const std::vector<RecipeType> &
RecipeDatabase::GetAllRecipeTypes()
{
    static const std::vector<RecipeType> recipeTypes{
        RecipeType::BRONZE_BAR,
        RecipeType::IRON_BAR,
        RecipeType::STEEL_BAR,
        RecipeType::WOODEN_PLANKS,
        RecipeType::WOODEN_SHIELD};

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

    static const RecipeDefinition woodenPlanksRecipe{
        RecipeType::WOODEN_PLANKS,
        "Craft Wooden planks",
        StationType::CRAFTING_TABLE,
        SkillType::NONE,
        0,
        0,
        1,
        {
            {ItemType::LOG, 1},
        },
        ItemType::WOODEN_PLANKS,
        4,
        CraftingPattern{
            CraftingPatternType::SHAPELESS,
            {ItemType::LOG}},
        false};

    static const RecipeDefinition woodenShieldRecipe{
        RecipeType::WOODEN_SHIELD,
        "Craft Wooden shield",
        StationType::CRAFTING_TABLE,
        SkillType::NONE,
        0,
        0,
        2,
        {
            {ItemType::WOODEN_PLANKS, 6},
        },
        ItemType::WOODEN_SHIELD,
        1,
        CraftingPattern{
            CraftingPatternType::SHAPED,
            {
                ItemType::WOODEN_PLANKS, ItemType::NONE, ItemType::WOODEN_PLANKS,
                ItemType::WOODEN_PLANKS, ItemType::WOODEN_PLANKS, ItemType::WOODEN_PLANKS,
                ItemType::NONE, ItemType::WOODEN_PLANKS, ItemType::NONE,
            }},
        false};

    switch (recipeType)
    {
    case RecipeType::BRONZE_BAR:
        return bronzeBarRecipe;

    case RecipeType::IRON_BAR:
        return ironBarRecipe;

    case RecipeType::STEEL_BAR:
        return steelBarRecipe;

    case RecipeType::WOODEN_PLANKS:
        return woodenPlanksRecipe;

    case RecipeType::WOODEN_SHIELD:
        return woodenShieldRecipe;

    case RecipeType::NONE:
    default:
        throw std::invalid_argument(
            "Unknown recipe type");
    }
}

const RecipeDefinition *RecipeDatabase::TryGet(
    RecipeType recipeType)
{
    switch (recipeType)
    {
    case RecipeType::BRONZE_BAR:
    case RecipeType::IRON_BAR:
    case RecipeType::STEEL_BAR:
    case RecipeType::WOODEN_PLANKS:
    case RecipeType::WOODEN_SHIELD:
        return &Get(recipeType);

    case RecipeType::NONE:
    default:
        return nullptr;
    }
}
