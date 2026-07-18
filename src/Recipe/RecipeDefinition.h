#pragma once

#include <string>
#include <vector>

#include "RecipeIngredient.h"
#include "RecipeType.h"
#include "../Inventory/ItemType.h"
#include "../Skills/SkillType.h"
#include "../World/Object/Station/StationType.h"
#include "../Requirement/Requirement.h"

class RecipeDefinition
{
public:
    RecipeDefinition(
        RecipeType recipeType,
        std::string name,
        StationType requiredStationType,
        SkillType requiredSkill,
        int requiredLevel,
        int xpReward,
        int actionDurationTicks,
        std::vector<RecipeIngredient> ingredients,
        ItemType outputItem,
        int outputAmount);

    RecipeType GetRecipeType() const;
    const std::string &GetName() const;

    StationType GetRequiredStationType() const;
    SkillType GetRequiredSkill() const;
    int GetRequiredLevel() const;
    int GetXPReward() const;
    int GetActionDurationTicks() const;

    const std::vector<RecipeIngredient> &
    GetIngredients() const;

    const std::vector<RequirementSystem::Requirement> &
    GetRequirements() const;

    ItemType GetOutputItem() const;
    int GetOutputAmount() const;

private:
    RecipeType recipeType;
    std::string name;

    StationType requiredStationType;
    SkillType requiredSkill;
    int requiredLevel;
    int xpReward;
    int actionDurationTicks;

    std::vector<RecipeIngredient> ingredients;

    std::vector<RequirementSystem::Requirement> requirements;

    ItemType outputItem;
    int outputAmount;
};
