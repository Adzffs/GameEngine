#include "RecipeSystem.h"

#include <vector>

#include "RecipeDatabase.h"
#include "../Player/Player.h"
#include "../Requirement/RequirementEvaluator.h"

bool RecipeSystem::CanCreateRecipe(
    const Player &player,
    RecipeType recipeType)
{
    const RecipeDefinition &recipe =
        RecipeDatabase::Get(recipeType);

    return RequirementSystem::RequirementEvaluator::EvaluateAll(
               player,
               recipe.GetRequirements())
        .satisfied;
}

bool RecipeSystem::TryCreateRecipe(
    Player &player,
    RecipeType recipeType)
{
    if (!CanCreateRecipe(
            player,
            recipeType))
    {
        return false;
    }

    const RecipeDefinition &recipe =
        RecipeDatabase::Get(recipeType);

    std::vector<ItemAmount> removals;
    removals.reserve(recipe.GetIngredients().size());

    for (const RecipeIngredient &ingredient : recipe.GetIngredients())
    {
        removals.push_back(
            ItemAmount{ingredient.itemType, ingredient.amount});
    }

    if (!player.GetInventory().TryApplyTransactionAtomically(
            removals,
            {ItemAmount{
                recipe.GetOutputItem(),
                recipe.GetOutputAmount()}}))
    {
        return false;
    }

    if (recipe.GetRequiredSkill() != SkillType::NONE &&
        recipe.GetXPReward() > 0)
    {
        player.GetSkills().AddXP(
            recipe.GetRequiredSkill(),
            recipe.GetXPReward());
    }

    return true;
}
