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
        RecipeType::WOODEN_SHIELD,
        RecipeType::BRONZE_AXE,
        RecipeType::IRON_AXE,
        RecipeType::STEEL_AXE,
        RecipeType::BRONZE_PICKAXE,
        RecipeType::IRON_PICKAXE,
        RecipeType::STEEL_PICKAXE,
        RecipeType::BRONZE_SWORD,
        RecipeType::OAK_PLANKS,
        RecipeType::WILLOW_PLANKS};

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

    static const RecipeDefinition bronzeAxeRecipe{
        RecipeType::BRONZE_AXE, "Forge Bronze axe", StationType::FURNACE,
        SkillType::SMITHING, 1, 12, 4,
        {{ItemType::BRONZE_BAR, 2}, {ItemType::WOODEN_PLANKS, 1}},
        ItemType::BRONZE_AXE, 1};
    static const RecipeDefinition ironAxeRecipe{
        RecipeType::IRON_AXE, "Forge Iron axe", StationType::FURNACE,
        SkillType::SMITHING, 10, 24, 5,
        {{ItemType::IRON_BAR, 2}, {ItemType::WOODEN_PLANKS, 1}},
        ItemType::IRON_AXE, 1};
    static const RecipeDefinition steelAxeRecipe{
        RecipeType::STEEL_AXE, "Forge Steel axe", StationType::FURNACE,
        SkillType::SMITHING, 20, 40, 6,
        {{ItemType::STEEL_BAR, 2}, {ItemType::WOODEN_PLANKS, 1}},
        ItemType::STEEL_AXE, 1};
    static const RecipeDefinition bronzePickaxeRecipe{
        RecipeType::BRONZE_PICKAXE, "Forge Bronze pickaxe", StationType::FURNACE,
        SkillType::SMITHING, 1, 16, 4,
        {{ItemType::BRONZE_BAR, 3}, {ItemType::WOODEN_PLANKS, 2}},
        ItemType::BRONZE_PICKAXE, 1};
    static const RecipeDefinition ironPickaxeRecipe{
        RecipeType::IRON_PICKAXE, "Forge Iron pickaxe", StationType::FURNACE,
        SkillType::SMITHING, 10, 32, 5,
        {{ItemType::IRON_BAR, 3}, {ItemType::WOODEN_PLANKS, 2}},
        ItemType::IRON_PICKAXE, 1};
    static const RecipeDefinition steelPickaxeRecipe{
        RecipeType::STEEL_PICKAXE, "Forge Steel pickaxe", StationType::FURNACE,
        SkillType::SMITHING, 20, 52, 6,
        {{ItemType::STEEL_BAR, 3}, {ItemType::WOODEN_PLANKS, 2}},
        ItemType::STEEL_PICKAXE, 1};
    static const RecipeDefinition bronzeSwordRecipe{
        RecipeType::BRONZE_SWORD, "Forge Bronze sword", StationType::FURNACE,
        SkillType::SMITHING, 1, 18, 4,
        {{ItemType::BRONZE_BAR, 3}, {ItemType::WOODEN_PLANKS, 1}},
        ItemType::BRONZE_SWORD, 1};
    static const RecipeDefinition oakPlanksRecipe{
        RecipeType::OAK_PLANKS, "Cut Oak planks", StationType::CRAFTING_TABLE,
        SkillType::WOODCUTTING, 5, 8, 1,
        {{ItemType::OAK_LOG, 1}}, ItemType::WOODEN_PLANKS, 6,
        CraftingPattern{CraftingPatternType::SHAPELESS, {ItemType::OAK_LOG}},
        false};
    static const RecipeDefinition willowPlanksRecipe{
        RecipeType::WILLOW_PLANKS, "Cut Willow planks", StationType::CRAFTING_TABLE,
        SkillType::WOODCUTTING, 10, 14, 1,
        {{ItemType::WILLOW_LOG, 1}}, ItemType::WOODEN_PLANKS, 8,
        CraftingPattern{CraftingPatternType::SHAPELESS, {ItemType::WILLOW_LOG}},
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

    case RecipeType::BRONZE_AXE: return bronzeAxeRecipe;
    case RecipeType::IRON_AXE: return ironAxeRecipe;
    case RecipeType::STEEL_AXE: return steelAxeRecipe;
    case RecipeType::BRONZE_PICKAXE: return bronzePickaxeRecipe;
    case RecipeType::IRON_PICKAXE: return ironPickaxeRecipe;
    case RecipeType::STEEL_PICKAXE: return steelPickaxeRecipe;
    case RecipeType::BRONZE_SWORD: return bronzeSwordRecipe;
    case RecipeType::OAK_PLANKS: return oakPlanksRecipe;
    case RecipeType::WILLOW_PLANKS: return willowPlanksRecipe;

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
    case RecipeType::BRONZE_AXE:
    case RecipeType::IRON_AXE:
    case RecipeType::STEEL_AXE:
    case RecipeType::BRONZE_PICKAXE:
    case RecipeType::IRON_PICKAXE:
    case RecipeType::STEEL_PICKAXE:
    case RecipeType::BRONZE_SWORD:
    case RecipeType::OAK_PLANKS:
    case RecipeType::WILLOW_PLANKS:
        return &Get(recipeType);

    case RecipeType::NONE:
    default:
        return nullptr;
    }
}
