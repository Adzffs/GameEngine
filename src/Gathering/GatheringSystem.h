#pragma once

#include "../Action/ActionValidationResult.h"
#include "../Action/Action.h"
#include "../Core/RandomSource.h"
#include "../Inventory/ItemType.h"
#include "../Skills/SkillType.h"

#include <functional>
#include <string>

class EntityManager;
class ObjectManager;

enum class GatheringCompletionOutcomeType
{
    IGNORE,
    CLEAR_STALE,
    CANCEL,
    FAILED_ROLL,
    SUCCESSFUL_ROLL
};

struct GatheringCompletionOutcome
{
    GatheringCompletionOutcomeType type =
        GatheringCompletionOutcomeType::IGNORE;
    int actorEntityID = 0;
    int targetResourceID = 0;
    ActionCancelReason cancelReason =
        ActionCancelReason::NONE;
    std::string message;

    std::string resourceName;
    SkillType requiredSkill = SkillType::NONE;
    ItemType rewardItem = ItemType::NONE;
    int rewardAmount = 0;
    int xpReward = 0;
};

class GatheringSystem
{
public:
    using CompletionValidator = std::function<ActionValidationResult(
        int actorEntityID,
        int resourceID,
        bool checkInventorySpace)>;

    GatheringCompletionOutcome EvaluateCompletedAction(
        const Action &action,
        const EntityManager &entityManager,
        const ObjectManager &objectManager,
        const CompletionValidator &completionValidator,
        RandomSource &gatheringRandomSource) const;
};
