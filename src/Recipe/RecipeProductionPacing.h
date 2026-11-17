#pragma once

#include "RecipeDefinition.h"

#include <algorithm>

class RecipeProductionPacing
{
public:
    static int GetDurationTicks(
        const RecipeDefinition &recipe,
        int skillLevel)
    {
        const int baseDuration = recipe.GetActionDurationTicks();
        if (recipe.GetRequiredSkill() == SkillType::NONE ||
            skillLevel <= recipe.GetRequiredLevel())
            return baseDuration;

        const int masterySteps =
            (skillLevel - recipe.GetRequiredLevel()) / 10;
        const int minimumDuration = std::max(1, (baseDuration + 1) / 2);
        return std::max(minimumDuration, baseDuration - masterySteps);
    }
};
