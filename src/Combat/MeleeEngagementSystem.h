#pragma once

#include "MeleeEngagementIntent.h"

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>

class EntityManager;

class MeleeEngagementSystem
{
public:
    bool RequestEngagement(
        int attackerEntityID,
        int targetEntityID,
        int durationTicks,
        const EntityManager &entityManager);

    bool ClearEngagement(int attackerEntityID);
    std::size_t ClearEngagementsInvolving(int entityID);
    bool HasEngagement(int attackerEntityID) const;
    std::optional<int> GetTargetEntityID(int attackerEntityID) const;
    std::size_t GetEngagementCount() const;

    std::vector<MeleeEngagementIntent> Evaluate(
        const EntityManager &entityManager);

private:
    struct Engagement
    {
        int targetEntityID;
        int durationTicks;
    };

    std::unordered_map<int, Engagement> engagementByAttackerID;
};
