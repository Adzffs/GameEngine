#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Action/ActionCancelReason.h"
#include "../src/Action/ActionType.h"
#include "../src/Core/RandomSource.h"
#include "../src/Entity/Manager/EntityManager.h"
#include "../src/Gathering/GatheringSystem.h"
#include "../src/World/Object/Manager/ObjectManager.h"
#include "../src/World/Object/Resource/ResourceDatabase.h"

#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
    class SequenceRandomSource : public RandomSource
    {
    public:
        explicit SequenceRandomSource(std::vector<int> values)
            : values(std::move(values))
        {
        }

        int NextIntInclusive(
            int minimum,
            int maximum) override
        {
            if (minimum > maximum)
            {
                int temporary = minimum;
                minimum = maximum;
                maximum = temporary;
            }

            if (minimum == maximum)
            {
                return minimum;
            }

            if (nextIndex >= static_cast<int>(values.size()))
            {
                throw std::runtime_error(
                    "SequenceRandomSource exhausted");
            }

            calls++;
            int value = values[nextIndex++];

            if (value < minimum)
            {
                return minimum;
            }

            if (value > maximum)
            {
                return maximum;
            }

            return value;
        }

        int GetCallCount() const
        {
            return calls;
        }

    private:
        std::vector<int> values;
        int nextIndex = 0;
        int calls = 0;
    };

    Action MakeGatheringAction(
        int ownerID,
        int targetResourceID)
    {
        return Action(
            ActionType::GATHERING,
            "Gathering",
            1,
            ownerID,
            targetResourceID,
            true);
    }
}

int main()
{
    TestContext test;
    GatheringSystem gatheringSystem;

    {
        EntityManager entityManager;
        ObjectManager objectManager;

        int playerID = entityManager.CreatePlayer();
        objectManager.CreateResource(
            ResourceType::NORMAL_TREE,
            5,
            5);

        ResourceNode *resource =
            objectManager.GetResourceAt(
                5,
                5);

        test.Expect(
            resource != nullptr,
            "Woodcutting resource exists for success evaluation");

        if (resource != nullptr)
        {
            int validatorCalls = 0;
            GatheringSystem::CompletionValidator validator =
                [&validatorCalls](
                    int,
                    int,
                    bool)
            {
                validatorCalls++;
                return ActionValidationResult{
                    true,
                    ActionCancelReason::NONE,
                    ""};
            };

            SequenceRandomSource random(
                std::vector<int>{1});

            GatheringCompletionOutcome outcome =
                gatheringSystem.EvaluateCompletedAction(
                    MakeGatheringAction(
                        playerID,
                        resource->GetID()),
                    entityManager,
                    objectManager,
                    validator,
                    random);

            const ResourceDefinition &definition =
                ResourceDatabase::Get(
                    resource->GetResourceType());

            test.ExpectEqual(
                static_cast<int>(outcome.type),
                static_cast<int>(GatheringCompletionOutcomeType::SUCCESSFUL_ROLL),
                "Successful roll returns a successful gathering outcome");
            test.ExpectEqual(
                random.GetCallCount(),
                1,
                "Successful evaluation consumes exactly one gathering roll");
            test.ExpectEqual(
                validatorCalls,
                1,
                "Successful evaluation runs completion validation once");
            test.ExpectEqual(
                static_cast<int>(outcome.requiredSkill),
                static_cast<int>(definition.GetRequiredSkill()),
                "Successful outcome carries the expected required skill");
            test.ExpectEqual(
                static_cast<int>(outcome.rewardItem),
                static_cast<int>(definition.GetItemReward()),
                "Successful outcome carries the expected reward item");
        }
    }

    {
        EntityManager entityManager;
        ObjectManager objectManager;

        int playerID = entityManager.CreatePlayer();
        objectManager.CreateResource(
            ResourceType::NORMAL_TREE,
            6,
            6);

        ResourceNode *resource =
            objectManager.GetResourceAt(
                6,
                6);

        test.Expect(
            resource != nullptr,
            "Woodcutting resource exists for failed-roll evaluation");

        if (resource != nullptr)
        {
            int validatorCalls = 0;
            GatheringSystem::CompletionValidator validator =
                [&validatorCalls](
                    int,
                    int,
                    bool)
            {
                validatorCalls++;
                return ActionValidationResult{
                    true,
                    ActionCancelReason::NONE,
                    ""};
            };

            SequenceRandomSource random(
                std::vector<int>{100});

            GatheringCompletionOutcome outcome =
                gatheringSystem.EvaluateCompletedAction(
                    MakeGatheringAction(
                        playerID,
                        resource->GetID()),
                    entityManager,
                    objectManager,
                    validator,
                    random);

            test.ExpectEqual(
                static_cast<int>(outcome.type),
                static_cast<int>(GatheringCompletionOutcomeType::FAILED_ROLL),
                "Failed roll returns a failed gathering outcome");
            test.ExpectEqual(
                random.GetCallCount(),
                1,
                "Failed evaluation still consumes exactly one gathering roll");
            test.ExpectEqual(
                validatorCalls,
                1,
                "Failed roll still validates completion once");
        }
    }

    {
        EntityManager entityManager;
        ObjectManager objectManager;

        int playerID = entityManager.CreatePlayer();
        objectManager.CreateResource(
            ResourceType::NORMAL_TREE,
            7,
            7);

        ResourceNode *resource =
            objectManager.GetResourceAt(
                7,
                7);

        test.Expect(
            resource != nullptr,
            "Woodcutting resource exists for invalid-tool cancellation");

        if (resource != nullptr)
        {
            SequenceRandomSource random(
                std::vector<int>{});

            GatheringCompletionOutcome outcome =
                gatheringSystem.EvaluateCompletedAction(
                    MakeGatheringAction(
                        playerID,
                        resource->GetID()),
                    entityManager,
                    objectManager,
                    [](int, int, bool)
                    {
                        return ActionValidationResult{
                            false,
                            ActionCancelReason::INVALID_TOOL,
                            "You need to equip the correct tool"};
                    },
                    random);

            test.ExpectEqual(
                static_cast<int>(outcome.type),
                static_cast<int>(GatheringCompletionOutcomeType::CANCEL),
                "Validation failure returns a cancellation outcome");
            test.ExpectEqual(
                static_cast<int>(outcome.cancelReason),
                static_cast<int>(ActionCancelReason::INVALID_TOOL),
                "Cancellation preserves INVALID_TOOL reason");
            test.ExpectEqual(
                random.GetCallCount(),
                0,
                "Validation cancellation consumes no gathering roll");
        }
    }

    {
        EntityManager entityManager;
        ObjectManager objectManager;

        int playerID = entityManager.CreatePlayer();

        int validatorCalls = 0;
        SequenceRandomSource random(
            std::vector<int>{});

        GatheringCompletionOutcome outcome =
            gatheringSystem.EvaluateCompletedAction(
                MakeGatheringAction(
                    playerID,
                    999999),
                entityManager,
                objectManager,
                [&validatorCalls](
                    int,
                    int,
                    bool)
                {
                    validatorCalls++;
                    return ActionValidationResult{
                        true,
                        ActionCancelReason::NONE,
                        ""};
                },
                random);

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(GatheringCompletionOutcomeType::CLEAR_STALE),
            "Missing targets are treated as stale completion state");
        test.ExpectEqual(
            validatorCalls,
            0,
            "Missing target skips completion validation");
        test.ExpectEqual(
            random.GetCallCount(),
            0,
            "Missing target consumes no gathering roll");
    }

    {
        EntityManager entityManager;
        ObjectManager objectManager;

        int playerID = entityManager.CreatePlayer();
        objectManager.CreateResource(
            ResourceType::NORMAL_TREE,
            8,
            8);

        ResourceNode *resource =
            objectManager.GetResourceAt(
                8,
                8);

        test.Expect(
            resource != nullptr,
            "Woodcutting resource exists for depleted-target handling");

        if (resource != nullptr)
        {
            resource->ConsumeUse();

            SequenceRandomSource random(
                std::vector<int>{});

            GatheringCompletionOutcome outcome =
                gatheringSystem.EvaluateCompletedAction(
                    MakeGatheringAction(
                        playerID,
                        resource->GetID()),
                    entityManager,
                    objectManager,
                    [](int, int, bool)
                    {
                        return ActionValidationResult{
                            true,
                            ActionCancelReason::NONE,
                            ""};
                    },
                    random);

            test.ExpectEqual(
                static_cast<int>(outcome.type),
                static_cast<int>(GatheringCompletionOutcomeType::CLEAR_STALE),
                "Depleted targets clear stale completion state without rewards");
            test.ExpectEqual(
                random.GetCallCount(),
                0,
                "Depleted targets consume no gathering roll");
        }
    }

    {
        EntityManager entityManager;
        ObjectManager objectManager;

        int playerID = entityManager.CreatePlayer();
        objectManager.CreateResource(
            ResourceType::NORMAL_TREE,
            9,
            9);

        ResourceNode *resource =
            objectManager.GetResourceAt(
                9,
                9);

        test.Expect(
            resource != nullptr,
            "Woodcutting resource exists for inventory-full cancellation");

        if (resource != nullptr)
        {
            SequenceRandomSource random(
                std::vector<int>{});

            GatheringCompletionOutcome outcome =
                gatheringSystem.EvaluateCompletedAction(
                    MakeGatheringAction(
                        playerID,
                        resource->GetID()),
                    entityManager,
                    objectManager,
                    [](int, int, bool)
                    {
                        return ActionValidationResult{
                            false,
                            ActionCancelReason::INVENTORY_FULL,
                            "Your inventory is full"};
                    },
                    random);

            test.ExpectEqual(
                static_cast<int>(outcome.type),
                static_cast<int>(GatheringCompletionOutcomeType::CANCEL),
                "Inventory-full validation returns cancellation outcome");
            test.ExpectEqual(
                static_cast<int>(outcome.cancelReason),
                static_cast<int>(ActionCancelReason::INVENTORY_FULL),
                "Inventory-full cancellation preserves reason");
            test.ExpectEqual(
                random.GetCallCount(),
                0,
                "Inventory-full cancellation consumes no gathering roll");
        }
    }

    {
        EntityManager entityManagerA;
        ObjectManager objectManagerA;
        int playerIDA = entityManagerA.CreatePlayer();
        objectManagerA.CreateResource(
            ResourceType::NORMAL_TREE,
            10,
            10);
        ResourceNode *resourceA = objectManagerA.GetResourceAt(
            10,
            10);

        EntityManager entityManagerB;
        ObjectManager objectManagerB;
        int playerIDB = entityManagerB.CreatePlayer();
        objectManagerB.CreateResource(
            ResourceType::NORMAL_TREE,
            11,
            11);
        ResourceNode *resourceB = objectManagerB.GetResourceAt(
            11,
            11);

        test.Expect(
            resourceA != nullptr && resourceB != nullptr,
            "Resources exist for deterministic repeatability checks");

        if (resourceA != nullptr && resourceB != nullptr)
        {
            GatheringSystem::CompletionValidator validator =
                [](int, int, bool)
            {
                return ActionValidationResult{
                    true,
                    ActionCancelReason::NONE,
                    ""};
            };

            SequenceRandomSource randomA(
                std::vector<int>{1, 100});
            SequenceRandomSource randomB(
                std::vector<int>{1, 100});

            GatheringCompletionOutcome firstA =
                gatheringSystem.EvaluateCompletedAction(
                    MakeGatheringAction(
                        playerIDA,
                        resourceA->GetID()),
                    entityManagerA,
                    objectManagerA,
                    validator,
                    randomA);
            GatheringCompletionOutcome secondA =
                gatheringSystem.EvaluateCompletedAction(
                    MakeGatheringAction(
                        playerIDA,
                        resourceA->GetID()),
                    entityManagerA,
                    objectManagerA,
                    validator,
                    randomA);

            GatheringCompletionOutcome firstB =
                gatheringSystem.EvaluateCompletedAction(
                    MakeGatheringAction(
                        playerIDB,
                        resourceB->GetID()),
                    entityManagerB,
                    objectManagerB,
                    validator,
                    randomB);
            GatheringCompletionOutcome secondB =
                gatheringSystem.EvaluateCompletedAction(
                    MakeGatheringAction(
                        playerIDB,
                        resourceB->GetID()),
                    entityManagerB,
                    objectManagerB,
                    validator,
                    randomB);

            test.ExpectEqual(
                static_cast<int>(firstA.type),
                static_cast<int>(firstB.type),
                "Deterministic script produces the same first completion outcome");
            test.ExpectEqual(
                static_cast<int>(secondA.type),
                static_cast<int>(secondB.type),
                "Deterministic script produces the same second completion outcome");
        }
    }

    {
        EntityManager entityManager;
        ObjectManager objectManager;

        int playerID = entityManager.CreatePlayer();
        objectManager.CreateResource(
            ResourceType::NORMAL_TREE,
            12,
            12);
        objectManager.CreateResource(
            ResourceType::COPPER_ROCK,
            13,
            13);

        ResourceNode *wood =
            objectManager.GetResourceAt(
                12,
                12);
        ResourceNode *copper =
            objectManager.GetResourceAt(
                13,
                13);

        test.Expect(
            wood != nullptr && copper != nullptr,
            "Woodcutting and mining resources exist for shared system checks");

        if (wood != nullptr && copper != nullptr)
        {
            GatheringSystem::CompletionValidator validator =
                [](int, int, bool)
            {
                return ActionValidationResult{
                    true,
                    ActionCancelReason::NONE,
                    ""};
            };

            SequenceRandomSource random(
                std::vector<int>{1, 1});

            GatheringCompletionOutcome woodOutcome =
                gatheringSystem.EvaluateCompletedAction(
                    MakeGatheringAction(
                        playerID,
                        wood->GetID()),
                    entityManager,
                    objectManager,
                    validator,
                    random);

            GatheringCompletionOutcome miningOutcome =
                gatheringSystem.EvaluateCompletedAction(
                    MakeGatheringAction(
                        playerID,
                        copper->GetID()),
                    entityManager,
                    objectManager,
                    validator,
                    random);

            test.ExpectEqual(
                static_cast<int>(woodOutcome.requiredSkill),
                static_cast<int>(SkillType::WOODCUTTING),
                "Woodcutting and mining both resolve through the shared gathering system");
            test.ExpectEqual(
                static_cast<int>(miningOutcome.requiredSkill),
                static_cast<int>(SkillType::MINING),
                "Mining uses the shared gathering system outcome model");
        }
    }

    return test.Finish();
}
