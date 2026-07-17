#include "RecipeSystem.h"

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
