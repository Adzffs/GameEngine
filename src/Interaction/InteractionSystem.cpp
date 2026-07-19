#include "InteractionSystem.h"

#include "../Entity/Manager/EntityManager.h"
#include "../Player/Player.h"
#include "../NPC/NPC.h"
#include "../NPC/NpcDefinitionDatabase.h"
#include "../Dialogue/DialogueDefinitionDatabase.h"
#include "../World/Object/Manager/ObjectManager.h"

#include <algorithm>
#include <cmath>

namespace
{
    bool TargetMatches(
        int targetObjectID,
        InteractionTargetType targetType,
        const ObjectManager &objects)
    {
        switch (targetType)
        {
        case InteractionTargetType::RESOURCE:
            return objects.GetResourceByID(targetObjectID) != nullptr;
        case InteractionTargetType::STATION:
            return objects.GetStationByID(targetObjectID) != nullptr;
        case InteractionTargetType::NPC:
            return false;
        }
        return false;
    }

    bool OppositeTargetExists(
        int targetObjectID,
        InteractionTargetType targetType,
        const ObjectManager &objects)
    {
        switch (targetType)
        {
        case InteractionTargetType::RESOURCE:
            return objects.GetStationByID(targetObjectID) != nullptr;
        case InteractionTargetType::STATION:
            return objects.GetResourceByID(targetObjectID) != nullptr;
        case InteractionTargetType::NPC:
            return false;
        }
        return false;
    }
}

bool InteractionSystem::RequestInteraction(
    int actorEntityID,
    int targetObjectID,
    InteractionTargetType targetType,
    const EntityManager &entityManager,
    const ObjectManager &objectManager)
{
    const Player *player = dynamic_cast<const Player *>(
        entityManager.GetEntityByID(actorEntityID));
    if (player == nullptr || !player->IsAlive() ||
        !TargetMatches(targetObjectID, targetType, objectManager))
    {
        return false;
    }

    pendingInteractionsByActorID.insert_or_assign(
        actorEntityID,
        PendingInteraction{actorEntityID, targetObjectID, targetType});
    return true;
}

bool InteractionSystem::RequestNpcInteraction(
    int actorEntityID, int targetNpcEntityID, NpcInteractionType interactionType,
    const EntityManager &entityManager)
{
    const Player *player = dynamic_cast<const Player *>(entityManager.GetEntityByID(actorEntityID));
    const NPC *npc = dynamic_cast<const NPC *>(entityManager.GetEntityByID(targetNpcEntityID));
    const NpcDefinition *definition = npc == nullptr ? nullptr :
        NpcDefinitionDatabase::TryGet(npc->GetNpcType());
    if (player == nullptr || !player->IsAlive() || npc == nullptr ||
        definition == nullptr || definition->kind != NpcKind::FRIENDLY ||
        !IsValidNpcInteractionType(interactionType) ||
        std::find(definition->interactions.begin(), definition->interactions.end(), interactionType) == definition->interactions.end() ||
        (interactionType == NpcInteractionType::TALK &&
         DialogueDefinitionDatabase::TryGet(definition->dialogueId) == nullptr))
        return false;
    pendingInteractionsByActorID.insert_or_assign(actorEntityID,
        PendingInteraction{actorEntityID, targetNpcEntityID, InteractionTargetType::NPC, interactionType});
    return true;
}

bool InteractionSystem::ClearInteraction(int actorEntityID)
{
    return pendingInteractionsByActorID.erase(actorEntityID) != 0;
}

std::size_t InteractionSystem::ClearInteractionsTargeting(int targetObjectID)
{
    std::size_t removed = 0;
    for (auto iterator = pendingInteractionsByActorID.begin();
         iterator != pendingInteractionsByActorID.end();)
    {
        if (iterator->second.targetObjectID == targetObjectID)
        {
            iterator = pendingInteractionsByActorID.erase(iterator);
            ++removed;
        }
        else
        {
            ++iterator;
        }
    }
    return removed;
}

bool InteractionSystem::HasInteraction(int actorEntityID) const
{
    return pendingInteractionsByActorID.contains(actorEntityID);
}

std::optional<PendingInteraction> InteractionSystem::GetInteraction(
    int actorEntityID) const
{
    const auto iterator = pendingInteractionsByActorID.find(actorEntityID);
    return iterator == pendingInteractionsByActorID.end()
               ? std::nullopt
               : std::optional<PendingInteraction>(iterator->second);
}

std::size_t InteractionSystem::GetInteractionCount() const
{
    return pendingInteractionsByActorID.size();
}

std::vector<InteractionIntent> InteractionSystem::Evaluate(
    const EntityManager &entityManager,
    const ObjectManager &objectManager,
    const std::unordered_set<int> &actorsWithActiveMovement)
{
    std::vector<InteractionIntent> intents;

    for (const auto &entityOwner : entityManager.GetEntities())
    {
        const int actorID = entityOwner->GetID();
        const auto iterator = pendingInteractionsByActorID.find(actorID);
        if (iterator == pendingInteractionsByActorID.end())
        {
            continue;
        }

        const PendingInteraction pending = iterator->second;
        const Player *player = dynamic_cast<const Player *>(entityOwner.get());
        InteractionClearReason reason = InteractionClearReason::NONE;
        if (player == nullptr)
        {
            reason = InteractionClearReason::INVALID_ACTOR;
        }
        else if (!player->IsAlive())
        {
            reason = InteractionClearReason::ACTOR_DEAD;
        }
        else if (pending.targetType == InteractionTargetType::NPC)
        {
            const NPC *npc = dynamic_cast<const NPC *>(entityManager.GetEntityByID(pending.targetObjectID));
            const NpcDefinition *definition = npc == nullptr ? nullptr : NpcDefinitionDatabase::TryGet(npc->GetNpcType());
            if (npc == nullptr || definition == nullptr ||
                definition->kind != NpcKind::FRIENDLY)
                reason = InteractionClearReason::INVALID_TARGET;
            else if (std::find(definition->interactions.begin(), definition->interactions.end(), pending.npcInteractionType) == definition->interactions.end())
                reason = InteractionClearReason::UNSUPPORTED_INTERACTION;
            else if (pending.npcInteractionType == NpcInteractionType::TALK &&
                     DialogueDefinitionDatabase::TryGet(definition->dialogueId) == nullptr)
                reason = InteractionClearReason::UNSUPPORTED_INTERACTION;
        }
        else if (!TargetMatches(
                     pending.targetObjectID, pending.targetType, objectManager))
        {
            reason = OppositeTargetExists(
                         pending.targetObjectID, pending.targetType, objectManager)
                         ? InteractionClearReason::TARGET_TYPE_MISMATCH
                         : InteractionClearReason::INVALID_TARGET;
        }

        if (reason != InteractionClearReason::NONE)
        {
            intents.push_back({actorID, pending.targetObjectID,
                               pending.targetType,
                               InteractionIntentType::CLEAR_INTERACTION, reason});
            pendingInteractionsByActorID.erase(iterator);
            continue;
        }

        int targetX = 0;
        int targetY = 0;
        if (pending.targetType == InteractionTargetType::NPC)
        {
            const NPC *npc = dynamic_cast<const NPC *>(entityManager.GetEntityByID(pending.targetObjectID));
            targetX = npc->GetPosition().GetX();
            targetY = npc->GetPosition().GetY();
        }
        else if (pending.targetType == InteractionTargetType::RESOURCE)
        {
            const auto *resource =
                objectManager.GetResourceByID(pending.targetObjectID);
            targetX = resource->GetX();
            targetY = resource->GetY();
        }
        else
        {
            const auto *station =
                objectManager.GetStationByID(pending.targetObjectID);
            targetX = station->GetX();
            targetY = station->GetY();
        }

        const bool inRange =
            std::abs(player->GetPosition().GetX() - targetX) <= 1 &&
            std::abs(player->GetPosition().GetY() - targetY) <= 1;
        if (inRange)
        {
            intents.push_back({actorID, pending.targetObjectID,
                               pending.targetType, InteractionIntentType::READY,
                               InteractionClearReason::NONE, pending.npcInteractionType});
            pendingInteractionsByActorID.erase(iterator);
        }
        else if (!actorsWithActiveMovement.contains(actorID))
        {
            intents.push_back({actorID, pending.targetObjectID,
                               pending.targetType,
                               InteractionIntentType::CLEAR_INTERACTION,
                               InteractionClearReason::APPROACH_FAILED});
            pendingInteractionsByActorID.erase(iterator);
        }
    }

    // Removed actors have no creation-order position. Clear them safely and
    // deterministically after all live actors.
    std::vector<int> missingActorIDs;
    for (const auto &[actorID, pending] : pendingInteractionsByActorID)
    {
        if (entityManager.GetEntityByID(actorID) == nullptr)
        {
            missingActorIDs.push_back(actorID);
        }
    }
    std::sort(missingActorIDs.begin(), missingActorIDs.end());
    for (int actorID : missingActorIDs)
    {
        const PendingInteraction pending =
            pendingInteractionsByActorID.at(actorID);
        intents.push_back({actorID, pending.targetObjectID, pending.targetType,
                           InteractionIntentType::CLEAR_INTERACTION,
                           InteractionClearReason::INVALID_ACTOR});
        pendingInteractionsByActorID.erase(actorID);
    }
    return intents;
}
