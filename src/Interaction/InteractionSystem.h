#pragma once

#include "InteractionIntent.h"

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class EntityManager;
class ObjectManager;

struct PendingInteraction
{
    int actorEntityID;
    int targetObjectID;
    InteractionTargetType targetType;

    bool operator==(const PendingInteraction &) const = default;
};

class InteractionSystem
{
public:
    bool RequestInteraction(
        int actorEntityID,
        int targetObjectID,
        InteractionTargetType targetType,
        const EntityManager &entityManager,
        const ObjectManager &objectManager);

    bool ClearInteraction(int actorEntityID);
    std::size_t ClearInteractionsTargeting(int targetObjectID);
    bool HasInteraction(int actorEntityID) const;
    std::optional<PendingInteraction> GetInteraction(int actorEntityID) const;
    std::size_t GetInteractionCount() const;

    std::vector<InteractionIntent> Evaluate(
        const EntityManager &entityManager,
        const ObjectManager &objectManager,
        const std::unordered_set<int> &actorsWithActiveMovement);

private:
    friend struct InteractionSystemTestAccess;
    std::unordered_map<int, PendingInteraction>
        pendingInteractionsByActorID;
};
