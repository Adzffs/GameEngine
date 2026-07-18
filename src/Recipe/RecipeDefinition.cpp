#include "RecipeDefinition.h"

#include <utility>

RecipeDefinition::RecipeDefinition(
    RecipeType recipeType,
    std::string name,
    StationType requiredStationType,
    SkillType requiredSkill,
    int requiredLevel,
    int xpReward,
    int actionDurationTicks,
    std::vector<RecipeIngredient> ingredients,
    ItemType outputItem,
    int outputAmount)
    : recipeType(recipeType),
      name(std::move(name)),
      requiredStationType(requiredStationType),
      requiredSkill(requiredSkill),
      requiredLevel(requiredLevel),
      xpReward(xpReward),
      actionDurationTicks(actionDurationTicks),
      ingredients(std::move(ingredients)),
      outputItem(outputItem),
      outputAmount(outputAmount)
{
    if (requiredSkill != SkillType::NONE ||
        requiredLevel != 0)
    {
        requirements.emplace_back(
            RequirementSystem::SkillLevelRequirement{
                requiredSkill,
                requiredLevel});
    }

    for (const RecipeIngredient &ingredient : this->ingredients)
    {
        requirements.emplace_back(
            RequirementSystem::HeldItemRequirement{
                ingredient.itemType,
                ingredient.amount});
    }
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

StationType
RecipeDefinition::GetRequiredStationType() const
{
    return requiredStationType;
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

int RecipeDefinition::GetActionDurationTicks() const
{
    return actionDurationTicks;
}

const std::vector<RecipeIngredient> &
RecipeDefinition::GetIngredients() const
{
    return ingredients;
}

const std::vector<RequirementSystem::Requirement> &
RecipeDefinition::GetRequirements() const
{
    return requirements;
}

ItemType RecipeDefinition::GetOutputItem() const
{
    return outputItem;
}

int RecipeDefinition::GetOutputAmount() const
{
    return outputAmount;
}
