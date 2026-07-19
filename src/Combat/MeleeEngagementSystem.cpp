#include "MeleeEngagementSystem.h"

#include "../Entity/Manager/EntityManager.h"
#include "../Entity/Monster/Monster.h"
#include "../Player/Player.h"

#include <cmath>

namespace
{
    const Combatant *GetSupportedCombatant(const Entity *entity)
    {
        if (dynamic_cast<const Player *>(entity) == nullptr &&
            dynamic_cast<const Monster *>(entity) == nullptr)
        {
            return nullptr;
        }
        return dynamic_cast<const Combatant *>(entity);
    }
}

bool MeleeEngagementSystem::RequestEngagement(
    int attackerEntityID,
    int targetEntityID,
    int durationTicks,
    const EntityManager &entityManager)
{
    if (attackerEntityID == targetEntityID || durationTicks < 1)
    {
        return false;
    }

    const Combatant *attacker = GetSupportedCombatant(
        entityManager.GetEntityByID(attackerEntityID));
    const Combatant *target = GetSupportedCombatant(
        entityManager.GetEntityByID(targetEntityID));

    if (attacker == nullptr || target == nullptr ||
        !attacker->IsAlive() || !target->IsAlive())
    {
        return false;
    }

    engagementByAttackerID.insert_or_assign(
        attackerEntityID,
        Engagement{targetEntityID, durationTicks});
    return true;
}

bool MeleeEngagementSystem::ClearEngagement(int attackerEntityID)
{
    return engagementByAttackerID.erase(attackerEntityID) != 0;
}

std::size_t MeleeEngagementSystem::ClearEngagementsInvolving(int entityID)
{
    std::size_t removedCount = engagementByAttackerID.erase(entityID);
    for (auto iterator = engagementByAttackerID.begin();
         iterator != engagementByAttackerID.end();)
    {
        if (iterator->second.targetEntityID == entityID)
        {
            iterator = engagementByAttackerID.erase(iterator);
            ++removedCount;
        }
        else
        {
            ++iterator;
        }
    }
    return removedCount;
}

bool MeleeEngagementSystem::HasEngagement(int attackerEntityID) const
{
    return engagementByAttackerID.contains(attackerEntityID);
}

std::optional<int> MeleeEngagementSystem::GetTargetEntityID(
    int attackerEntityID) const
{
    auto iterator = engagementByAttackerID.find(attackerEntityID);
    return iterator == engagementByAttackerID.end()
               ? std::nullopt
               : std::optional<int>(iterator->second.targetEntityID);
}

std::size_t MeleeEngagementSystem::GetEngagementCount() const
{
    return engagementByAttackerID.size();
}

std::vector<MeleeEngagementIntent> MeleeEngagementSystem::Evaluate(
    const EntityManager &entityManager)
{
    std::vector<MeleeEngagementIntent> intents;

    for (const auto &entityOwner : entityManager.GetEntities())
    {
        const int attackerEntityID = entityOwner->GetID();
        auto engagementIterator =
            engagementByAttackerID.find(attackerEntityID);
        if (engagementIterator == engagementByAttackerID.end())
        {
            continue;
        }

        const Engagement engagement = engagementIterator->second;
        const Entity *attackerEntity = entityOwner.get();
        const Entity *targetEntity =
            entityManager.GetEntityByID(engagement.targetEntityID);
        const Combatant *attacker = GetSupportedCombatant(attackerEntity);
        const Combatant *target = GetSupportedCombatant(targetEntity);

        MeleeEngagementClearReason clearReason =
            MeleeEngagementClearReason::NONE;
        if (attacker == nullptr)
        {
            clearReason = MeleeEngagementClearReason::INVALID_ATTACKER;
        }
        else if (!attacker->IsAlive())
        {
            clearReason = MeleeEngagementClearReason::ATTACKER_DEAD;
        }
        else if (target == nullptr)
        {
            clearReason = MeleeEngagementClearReason::INVALID_TARGET;
        }
        else if (!target->IsAlive())
        {
            clearReason = MeleeEngagementClearReason::TARGET_DEAD;
        }

        if (clearReason != MeleeEngagementClearReason::NONE)
        {
            intents.push_back(MeleeEngagementIntent{
                attackerEntityID,
                engagement.targetEntityID,
                engagement.durationTicks,
                MeleeEngagementIntentType::CLEAR_ENGAGEMENT,
                clearReason});
            engagementByAttackerID.erase(engagementIterator);
            continue;
        }

        const int distanceX = std::abs(
            attackerEntity->GetPosition().GetX() -
            targetEntity->GetPosition().GetX());
        const int distanceY = std::abs(
            attackerEntity->GetPosition().GetY() -
            targetEntity->GetPosition().GetY());
        const bool isAdjacent = distanceX <= 1 && distanceY <= 1;

        intents.push_back(MeleeEngagementIntent{
            attackerEntityID,
            engagement.targetEntityID,
            engagement.durationTicks,
            isAdjacent ? MeleeEngagementIntentType::ATTACK_TARGET
                       : MeleeEngagementIntentType::APPROACH_TARGET,
            MeleeEngagementClearReason::NONE});
    }

    for (auto iterator = engagementByAttackerID.begin();
         iterator != engagementByAttackerID.end();)
    {
        if (entityManager.GetEntityByID(iterator->first) == nullptr)
        {
            iterator = engagementByAttackerID.erase(iterator);
        }
        else
        {
            ++iterator;
        }
    }

    return intents;
}
