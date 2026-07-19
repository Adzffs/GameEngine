#pragma once

#include "RecipeIngredient.h"
#include "RecipeType.h"
#include "../Action/Action.h"
#include "../Action/ActionValidationResult.h"
#include "../Inventory/ItemType.h"
#include "../Skills/SkillType.h"

#include <functional>
#include <string>
#include <vector>

class EntityManager;

enum class RecipeActionCompletionOutcomeType
{
    IGNORE,
    CLEAR_STALE,
    CANCEL,
    COMPLETE
};

struct RecipeActionCompletionOutcome
{
    RecipeActionCompletionOutcomeType type =
        RecipeActionCompletionOutcomeType::IGNORE;
    int actorEntityID = 0;
    RecipeType recipeType = RecipeType::NONE;
    ActionCancelReason cancelReason =
        ActionCancelReason::NONE;
    std::string message;

    std::string recipeName;
    SkillType requiredSkill = SkillType::NONE;
    int xpReward = 0;
    std::vector<RecipeIngredient> ingredients;
    ItemType outputItem = ItemType::NONE;
    int outputAmount = 0;
};

class RecipeActionSystem
{
public:
    using CompletionValidator = std::function<ActionValidationResult(
        int actorEntityID,
        RecipeType recipeType)>;

    RecipeActionCompletionOutcome EvaluateCompletedAction(
        const Action &action,
        const EntityManager &entityManager,
        const CompletionValidator &completionValidator) const;
};
