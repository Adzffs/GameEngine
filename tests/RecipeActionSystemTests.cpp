#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Action/ActionCancelReason.h"
#include "../src/Action/ActionValidationResult.h"
#include "../src/Action/ActionType.h"
#include "../src/Entity/Manager/EntityManager.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Item/ItemDatabase.h"
#include "../src/Recipe/RecipeActionSystem.h"
#include "../src/Recipe/RecipeDatabase.h"
#include "../src/Recipe/RecipeType.h"
#include "../src/Skills/SkillType.h"

#include <array>

namespace
{
    Action MakeRecipeAction(
        int actorEntityID,
        RecipeType recipeType)
    {
        return Action(
            ActionType::RECIPE,
            "Recipe",
            3,
            actorEntityID,
            static_cast<int>(recipeType),
            true);
    }
}

int main()
{
    TestContext test;
    RecipeActionSystem recipeActionSystem;

    {
        EntityManager entities;
        int playerID = entities.CreatePlayer();

        int validatorCalls = 0;

        RecipeActionCompletionOutcome outcome =
            recipeActionSystem.EvaluateCompletedAction(
                MakeRecipeAction(
                    playerID,
                    RecipeType::BRONZE_BAR),
                entities,
                [&](int actorEntityID, RecipeType recipeType)
                {
                    validatorCalls++;
                    test.ExpectEqual(
                        actorEntityID,
                        playerID,
                        "Validator receives the action owner ID");
                    test.ExpectEqual(
                        static_cast<int>(recipeType),
                        static_cast<int>(RecipeType::BRONZE_BAR),
                        "Validator receives the recipe ID from the completed action");

                    return ActionValidationResult{
                        true,
                        ActionCancelReason::NONE,
                        ""};
                });

        const RecipeDefinition &recipe =
            RecipeDatabase::Get(RecipeType::BRONZE_BAR);

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(RecipeActionCompletionOutcomeType::COMPLETE),
            "Valid bronze completion returns a complete outcome");
        test.ExpectEqual(
            outcome.actorEntityID,
            playerID,
            "Complete outcome preserves actor ID");
        test.ExpectEqual(
            static_cast<int>(outcome.recipeType),
            static_cast<int>(RecipeType::BRONZE_BAR),
            "Complete outcome preserves recipe ID");
        test.ExpectEqual(
            outcome.recipeName,
            recipe.GetName(),
            "Complete outcome uses authoritative recipe name");
        test.ExpectEqual(
            static_cast<int>(outcome.requiredSkill),
            static_cast<int>(recipe.GetRequiredSkill()),
            "Complete outcome uses authoritative required skill");
        test.ExpectEqual(
            outcome.xpReward,
            recipe.GetXPReward(),
            "Complete outcome uses authoritative XP reward");
        test.ExpectEqual(
            static_cast<int>(outcome.outputItem),
            static_cast<int>(recipe.GetOutputItem()),
            "Complete outcome uses authoritative output item");
        test.ExpectEqual(
            outcome.outputAmount,
            recipe.GetOutputAmount(),
            "Complete outcome uses authoritative output amount");
        test.ExpectEqual(
            static_cast<int>(outcome.ingredients.size()),
            static_cast<int>(recipe.GetIngredients().size()),
            "Complete outcome includes authoritative ingredient count");

        if (outcome.ingredients.size() == recipe.GetIngredients().size())
        {
            for (int index = 0;
                 index < static_cast<int>(outcome.ingredients.size());
                 ++index)
            {
                test.ExpectEqual(
                    static_cast<int>(outcome.ingredients[index].itemType),
                    static_cast<int>(recipe.GetIngredients()[index].itemType),
                    "Complete outcome ingredient item type matches authoritative recipe");
                test.ExpectEqual(
                    outcome.ingredients[index].amount,
                    recipe.GetIngredients()[index].amount,
                    "Complete outcome ingredient quantity matches authoritative recipe");
            }
        }

        test.ExpectEqual(
            validatorCalls,
            1,
            "Valid completion evaluates requirements exactly once");
    }

    {
        EntityManager entities;
        int playerID = entities.CreatePlayer();

        const std::array<RecipeType, 2> recipeTypes{
            RecipeType::IRON_BAR,
            RecipeType::STEEL_BAR};

        for (RecipeType recipeType : recipeTypes)
        {
            RecipeActionCompletionOutcome outcome =
                recipeActionSystem.EvaluateCompletedAction(
                    MakeRecipeAction(
                        playerID,
                        recipeType),
                    entities,
                    [](int, RecipeType)
                    {
                        return ActionValidationResult{
                            true,
                            ActionCancelReason::NONE,
                            ""};
                    });

            const RecipeDefinition &recipe =
                RecipeDatabase::Get(recipeType);

            test.ExpectEqual(
                static_cast<int>(outcome.type),
                static_cast<int>(RecipeActionCompletionOutcomeType::COMPLETE),
                "Iron and steel completions follow the shared evaluator path");
            test.ExpectEqual(
                static_cast<int>(outcome.outputItem),
                static_cast<int>(recipe.GetOutputItem()),
                "Complete outcome uses authoritative output item for each recipe");
            test.ExpectEqual(
                outcome.outputAmount,
                recipe.GetOutputAmount(),
                "Complete outcome uses authoritative output amount for each recipe");
        }
    }

    {
        const std::array<RecipeType, 3> recipeTypes{
            RecipeType::BRONZE_BAR,
            RecipeType::IRON_BAR,
            RecipeType::STEEL_BAR};

        for (RecipeType recipeType : recipeTypes)
        {
            const RecipeDefinition &recipe =
                RecipeDatabase::Get(recipeType);

            int removableSlotCount = 0;

            for (const RecipeIngredient &ingredient :
                 recipe.GetIngredients())
            {
                test.Expect(
                    !ItemDatabase::Get(
                         ingredient.itemType)
                         .IsStackable(),
                    "Authoritative smithing ingredients are non-stackable");

                removableSlotCount +=
                    ingredient.amount;
            }

            test.Expect(
                !ItemDatabase::Get(
                     recipe.GetOutputItem())
                     .IsStackable(),
                "Authoritative smithing outputs are non-stackable");
            test.Expect(
                removableSlotCount >=
                    recipe.GetOutputAmount(),
                "Recipe ingredients free enough slots for output once removed");
        }
    }

    {
        EntityManager entities;

        int validatorCalls = 0;

        RecipeActionCompletionOutcome outcome =
            recipeActionSystem.EvaluateCompletedAction(
                MakeRecipeAction(
                    999999,
                    RecipeType::BRONZE_BAR),
                entities,
                [&](int, RecipeType)
                {
                    validatorCalls++;
                    return ActionValidationResult{
                        true,
                        ActionCancelReason::NONE,
                        ""};
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(RecipeActionCompletionOutcomeType::CLEAR_STALE),
            "Missing actor returns clear-stale outcome");
        test.ExpectEqual(
            validatorCalls,
            0,
            "Missing actor does not call completion validator");
    }

    {
        EntityManager entities;
        int playerID = entities.CreatePlayer();

        int validatorCalls = 0;

        Action invalidRecipeAction(
            ActionType::RECIPE,
            "Recipe",
            3,
            playerID,
            999999,
            true);

        RecipeActionCompletionOutcome outcome =
            recipeActionSystem.EvaluateCompletedAction(
                invalidRecipeAction,
                entities,
                [&](int, RecipeType)
                {
                    validatorCalls++;
                    return ActionValidationResult{
                        true,
                        ActionCancelReason::NONE,
                        ""};
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(RecipeActionCompletionOutcomeType::CANCEL),
            "Invalid recipe ID returns cancellation outcome");
        test.ExpectEqual(
            static_cast<int>(outcome.cancelReason),
            static_cast<int>(ActionCancelReason::REQUIREMENTS_FAILED),
            "Invalid recipe ID uses requirements-failed cancellation reason");
        test.ExpectEqual(
            validatorCalls,
            0,
            "Invalid recipe ID does not call completion validator");
    }

    {
        EntityManager entities;
        int playerID = entities.CreatePlayer();

        RecipeActionCompletionOutcome outcome =
            recipeActionSystem.EvaluateCompletedAction(
                MakeRecipeAction(
                    playerID,
                    RecipeType::BRONZE_BAR),
                entities,
                [](int, RecipeType)
                {
                    return ActionValidationResult{
                        false,
                        ActionCancelReason::INVENTORY_FULL,
                        "Your inventory does not have enough space"};
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(RecipeActionCompletionOutcomeType::CANCEL),
            "Inventory-full validation returns cancellation outcome");
        test.ExpectEqual(
            static_cast<int>(outcome.cancelReason),
            static_cast<int>(ActionCancelReason::INVENTORY_FULL),
            "Inventory-full validation preserves cancellation reason");
        test.ExpectEqual(
            outcome.recipeName,
            std::string(""),
            "Cancelled completion does not expose completion payload values");
        test.ExpectEqual(
            static_cast<int>(outcome.ingredients.size()),
            0,
            "Cancelled completion does not expose ingredient payload values");
    }

    {
        EntityManager entities;

        int validatorCalls = 0;

        Action nonRecipeAction(
            ActionType::GATHERING,
            "Gathering",
            3,
            1,
            2,
            true);

        RecipeActionCompletionOutcome outcome =
            recipeActionSystem.EvaluateCompletedAction(
                nonRecipeAction,
                entities,
                [&](int, RecipeType)
                {
                    validatorCalls++;
                    return ActionValidationResult{
                        true,
                        ActionCancelReason::NONE,
                        ""};
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(RecipeActionCompletionOutcomeType::IGNORE),
            "Non-recipe completed action is ignored");
        test.ExpectEqual(
            validatorCalls,
            0,
            "Ignored actions do not call the completion validator");
    }

    return test.Finish();
}
