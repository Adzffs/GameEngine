#include "RecipeActionSystem.h"

#include "RecipeDatabase.h"
#include "../Entity/Entity.h"
#include "../Entity/Manager/EntityManager.h"
#include "../Player/Player.h"

namespace
{
    bool IsSupportedRecipeType(
        RecipeType recipeType)
    {
        switch (recipeType)
        {
        case RecipeType::BRONZE_BAR:
        case RecipeType::IRON_BAR:
        case RecipeType::STEEL_BAR:
            return true;

        case RecipeType::NONE:
        default:
            return false;
        }
    }
}

RecipeActionCompletionOutcome RecipeActionSystem::EvaluateCompletedAction(
    const Action &action,
    const EntityManager &entityManager,
    const CompletionValidator &completionValidator) const
{
    if (action.GetType() != ActionType::RECIPE)
    {
        return {};
    }

    RecipeActionCompletionOutcome outcome;
    outcome.actorEntityID = action.GetOwnerID();
    outcome.recipeType = static_cast<RecipeType>(
        action.GetTargetID());

    const Entity *entity =
        entityManager.GetEntityByID(
            action.GetOwnerID());

    const Player *player =
        dynamic_cast<const Player *>(entity);

    if (player == nullptr)
    {
        outcome.type =
            RecipeActionCompletionOutcomeType::CLEAR_STALE;
        return outcome;
    }

    if (!IsSupportedRecipeType(outcome.recipeType))
    {
        outcome.type =
            RecipeActionCompletionOutcomeType::CANCEL;
        outcome.cancelReason =
            ActionCancelReason::REQUIREMENTS_FAILED;
        outcome.message =
            "The selected recipe is invalid";
        return outcome;
    }

    ActionValidationResult validation =
        completionValidator(
            action.GetOwnerID(),
            outcome.recipeType);

    if (!validation.valid)
    {
        outcome.type =
            RecipeActionCompletionOutcomeType::CANCEL;
        outcome.cancelReason =
            validation.reason;
        outcome.message =
            validation.message;
        return outcome;
    }

    const RecipeDefinition &recipe =
        RecipeDatabase::Get(
            outcome.recipeType);

    outcome.type =
        RecipeActionCompletionOutcomeType::COMPLETE;
    outcome.recipeName = recipe.GetName();
    outcome.requiredSkill = recipe.GetRequiredSkill();
    outcome.xpReward = recipe.GetXPReward();
    outcome.ingredients = recipe.GetIngredients();
    outcome.outputItem = recipe.GetOutputItem();
    outcome.outputAmount = recipe.GetOutputAmount();

    return outcome;
}
