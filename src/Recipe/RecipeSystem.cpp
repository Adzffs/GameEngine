#include "RecipeSystem.h"

#include <vector>

#include "RecipeDatabase.h"
#include "../Player/Player.h"

bool RecipeSystem::CanCreateRecipe(
    const Player &player,
    RecipeType recipeType)
{
    const RecipeDefinition &recipe =
        RecipeDatabase::Get(recipeType);

    const Skill &requiredSkill =
        player.GetSkills()
            .GetSkill(recipe.GetRequiredSkill());

    if (requiredSkill.GetLevel() <
        recipe.GetRequiredLevel())
    {
        return false;
    }

    const Inventory &inventory =
        player.GetInventory();

    for (const RecipeIngredient &ingredient :
         recipe.GetIngredients())
    {
        if (inventory.GetItemAmount(
                ingredient.itemType) <
            ingredient.amount)
        {
            return false;
        }
    }

    return true;
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

    Inventory &inventory =
        player.GetInventory();

    std::vector<RecipeIngredient>
        removedIngredients;

    auto restoreIngredients = [&]()
    {
        for (const RecipeIngredient &ingredient :
             removedIngredients)
        {
            inventory.AddItem(
                ingredient.itemType,
                ingredient.amount);
        }
    };

    for (const RecipeIngredient &ingredient :
         recipe.GetIngredients())
    {
        bool removed =
            inventory.RemoveItem(
                ingredient.itemType,
                ingredient.amount);

        if (!removed)
        {
            restoreIngredients();
            return false;
        }

        removedIngredients.push_back(
            ingredient);
    }

    bool outputAdded =
        inventory.AddItem(
            recipe.GetOutputItem(),
            recipe.GetOutputAmount());

    if (!outputAdded)
    {
        restoreIngredients();
        return false;
    }

    player.GetSkills().AddXP(
        recipe.GetRequiredSkill(),
        recipe.GetXPReward());

    return true;
}
