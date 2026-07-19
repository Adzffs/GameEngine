#include "GatheringSystem.h"

#include "../Entity/Entity.h"
#include "../Entity/Manager/EntityManager.h"
#include "../Player/Player.h"
#include "../World/Object/Manager/ObjectManager.h"
#include "../World/Object/Resource/ResourceDatabase.h"
#include "../World/Object/Resource/ResourceNode.h"

GatheringCompletionOutcome GatheringSystem::EvaluateCompletedAction(
    const Action &action,
    const EntityManager &entityManager,
    const ObjectManager &objectManager,
    const CompletionValidator &completionValidator,
    RandomSource &gatheringRandomSource) const
{
    if (action.GetType() != ActionType::GATHERING)
    {
        return {};
    }

    GatheringCompletionOutcome outcome;
    outcome.actorEntityID =
        action.GetOwnerID();
    outcome.targetResourceID =
        action.GetTargetID();

    const ResourceNode *resource =
        objectManager.GetResourceByID(
            action.GetTargetID());

    if (resource == nullptr ||
        !resource->IsActive())
    {
        outcome.type =
            GatheringCompletionOutcomeType::CLEAR_STALE;
        return outcome;
    }

    const Entity *entity =
        entityManager.GetEntityByID(
            action.GetOwnerID());

    const Player *player =
        dynamic_cast<const Player *>(entity);

    if (player == nullptr)
    {
        outcome.type =
            GatheringCompletionOutcomeType::CLEAR_STALE;
        return outcome;
    }

    ActionValidationResult validation =
        completionValidator(
            action.GetOwnerID(),
            action.GetTargetID(),
            true);

    if (!validation.valid)
    {
        outcome.type =
            GatheringCompletionOutcomeType::CANCEL;
        outcome.cancelReason =
            validation.reason;
        outcome.message =
            validation.message;
        return outcome;
    }

    const ResourceDefinition &resourceDefinition =
        ResourceDatabase::Get(
            resource->GetResourceType());

    const SkillType requiredSkill =
        resourceDefinition.GetRequiredSkill();

    const int playerSkillLevel =
        player->GetSkills()
            .GetSkill(requiredSkill)
            .GetLevel();

    const int levelsAboveRequirement =
        playerSkillLevel -
        resourceDefinition
            .GetRequiredSkillLevel();

    int successChance =
        resourceDefinition
            .GetBaseSuccessChance() +
        levelsAboveRequirement * 2;

    if (successChance > 95)
    {
        successChance = 95;
    }

    const bool successfulGather =
        gatheringRandomSource.RollPercentage(
            successChance);

    outcome.resourceName =
        resourceDefinition.GetName();
    outcome.requiredSkill =
        requiredSkill;
    outcome.rewardItem =
        resourceDefinition.GetItemReward();
    outcome.rewardAmount =
        resourceDefinition.GetItemAmount();
    outcome.xpReward =
        resourceDefinition.GetXPReward();

    outcome.type =
        successfulGather
            ? GatheringCompletionOutcomeType::SUCCESSFUL_ROLL
            : GatheringCompletionOutcomeType::FAILED_ROLL;

    return outcome;
}
