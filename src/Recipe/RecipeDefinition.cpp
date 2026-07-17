#include "RecipeDefinition.h"

#include <utility>

RecipeDefinition::RecipeDefinition(
    RecipeType recipeType,
    std::string name,
    SkillType requiredSkill,
    int requiredLevel,
    int xpReward,
    std::vector<RecipeIngredient> ingredients,
    ItemType outputItem,
    int outputAmount)
    : recipeType(recipeType),
      name(std::move(name)),
      requiredSkill(requiredSkill),
      requiredLevel(requiredLevel),
      xpReward(xpReward),
      ingredients(std::move(ingredients)),
      outputItem(outputItem),
      outputAmount(outputAmount)
{
}

RecipeType RecipeDefinition::GetRecipeType() const
{
    return recipeType;
}

const std::string &
RecipeDefinition::GetName() const
{
    return name;
}

SkillType RecipeDefinition::GetRequiredSkill() const
{
    return requiredSkill;
}

int RecipeDefinition::GetRequiredLevel() const
{
    return requiredLevel;
}

int RecipeDefinition::GetXPReward() const
{
    return xpReward;
}

const std::vector<RecipeIngredient> &
RecipeDefinition::GetIngredients() const
{
    return ingredients;
}

ItemType RecipeDefinition::GetOutputItem() const
{
    return outputItem;
}

int RecipeDefinition::GetOutputAmount() const
{
    return outputAmount;
}
