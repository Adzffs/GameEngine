#pragma once

#include <string>
#include <vector>

#include "RecipeIngredient.h"
#include "RecipeType.h"
#include "../Inventory/ItemType.h"
#include "../Skills/SkillType.h"

class RecipeDefinition
{
public:
    RecipeDefinition(
        RecipeType recipeType,
        std::string name,
        SkillType requiredSkill,
        int requiredLevel,
        int xpReward,
        int actionDurationTicks,
        std::vector<RecipeIngredient> ingredients,
        ItemType outputItem,
        int outputAmount);

    RecipeType GetRecipeType() const;
    const std::string &GetName() const;

    SkillType GetRequiredSkill() const;
    int GetRequiredLevel() const;
    int GetXPReward() const;
    int GetActionDurationTicks() const;

    const std::vector<RecipeIngredient> &
    GetIngredients() const;

    ItemType GetOutputItem() const;
    int GetOutputAmount() const;

private:
    RecipeType recipeType;
    std::string name;

    SkillType requiredSkill;
    int requiredLevel;
    int xpReward;
    int actionDurationTicks;

    std::vector<RecipeIngredient> ingredients;

    ItemType outputItem;
    int outputAmount;
};
