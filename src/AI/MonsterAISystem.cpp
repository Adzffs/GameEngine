#include "MonsterAISystem.h"

#include "../Entity/Manager/EntityManager.h"
#include "../Entity/Monster/Monster.h"
#include "../Player/Player.h"
#include "../World/Distance.h"

#include <cstdlib>
#include <optional>

namespace
{
    bool IsAdjacent(
        const Monster &monster,
        const Player &player)
    {
        const int distanceX = std::abs(
            monster.GetPosition().GetX() -
            player.GetPosition().GetX());
        const int distanceY = std::abs(
            monster.GetPosition().GetY() -
            player.GetPosition().GetY());

        return distanceX <= 1 && distanceY <= 1;
    }

    bool IsOutsideLeash(
        const Monster &monster,
        const MonsterAggressionDefinition &definition)
    {
        return Distance::Calculate(
                   monster.GetPosition().GetX(),
                   monster.GetPosition().GetY(),
                   monster.GetOriginalSpawnX(),
                   monster.GetOriginalSpawnY()) >
               definition.leashRadius;
    }
}

bool MonsterAISystem::IsValidTarget(
    const Monster &monster,
    const EntityManager &entityManager,
    int targetEntityID) const
{
    if (!monster.IsAlive() ||
        !monster.HasAggressionDefinition() ||
        targetEntityID == Monster::InvalidAggressionTargetEntityID)
    {
        return false;
    }

    const std::optional<MonsterAggressionDefinition> definition =
        monster.GetAggressionDefinition();
    const Player *player = dynamic_cast<const Player *>(
        entityManager.GetEntityByID(targetEntityID));

    if (!definition.has_value() ||
        player == nullptr ||
        !player->IsAlive() ||
        IsOutsideLeash(monster, definition.value()))
    {
        return false;
    }

    return Distance::Calculate(
               player->GetPosition().GetX(),
               player->GetPosition().GetY(),
               monster.GetOriginalSpawnX(),
               monster.GetOriginalSpawnY()) <=
           definition->leashRadius;
}

MonsterAIIntent MonsterAISystem::Evaluate(
    const Monster &monster,
    const EntityManager &entityManager,
    bool respawnSuppressed) const
{
    MonsterAIIntent intent;
    intent.monsterEntityID = monster.GetID();

    if (!monster.IsAlive() || respawnSuppressed)
    {
        return intent;
    }

    const std::optional<MonsterAggressionDefinition> definition =
        monster.GetAggressionDefinition();
    if (!definition.has_value() ||
        definition->detectionRadius <= 0 ||
        definition->leashRadius < definition->detectionRadius)
    {
        return intent;
    }

    const int currentTargetID =
        monster.GetAggressionTargetEntityID();
    bool currentTargetInvalid = false;
    if (currentTargetID != Monster::InvalidAggressionTargetEntityID)
    {
        if (!IsValidTarget(monster, entityManager, currentTargetID))
        {
            currentTargetInvalid = true;
            if (IsOutsideLeash(monster, definition.value()))
            {
                intent.type = MonsterAIIntentType::CLEAR_TARGET;
                intent.targetEntityID = currentTargetID;
                intent.clearReason = MonsterAIClearReason::LEASH_VIOLATED;
                return intent;
            }
        }
        else
        {
            const Player *player = dynamic_cast<const Player *>(
                entityManager.GetEntityByID(currentTargetID));
            intent.type = IsAdjacent(monster, *player)
                              ? MonsterAIIntentType::ATTACK_TARGET
                              : MonsterAIIntentType::CHASE_TARGET;
            intent.targetEntityID = currentTargetID;
            intent.destination = player->GetPosition();
            return intent;
        }
    }

    struct Candidate
    {
        const Player *player;
        int distance;
    };

    std::optional<Candidate> selected;
    for (const std::unique_ptr<Entity> &entity :
         entityManager.GetEntities())
    {
        const Player *player =
            dynamic_cast<const Player *>(entity.get());
        if (player == nullptr || !player->IsAlive())
        {
            continue;
        }

        const int distance = Distance::Calculate(
            monster.GetPosition().GetX(),
            monster.GetPosition().GetY(),
            player->GetPosition().GetX(),
            player->GetPosition().GetY());
        const int distanceFromSpawn = Distance::Calculate(
            monster.GetOriginalSpawnX(),
            monster.GetOriginalSpawnY(),
            player->GetPosition().GetX(),
            player->GetPosition().GetY());

        if (distance > definition->detectionRadius ||
            distanceFromSpawn > definition->leashRadius)
        {
            continue;
        }

        if (!selected.has_value() ||
            distance < selected->distance ||
            (distance == selected->distance &&
             player->GetID() < selected->player->GetID()))
        {
            selected = Candidate{player, distance};
        }
    }

    if (selected.has_value())
    {
        intent.type = IsAdjacent(monster, *selected->player)
                          ? MonsterAIIntentType::ATTACK_TARGET
                          : MonsterAIIntentType::CHASE_TARGET;
        intent.targetEntityID = selected->player->GetID();
        intent.destination = selected->player->GetPosition();
        return intent;
    }

    if (currentTargetInvalid)
    {
        intent.type = MonsterAIIntentType::CLEAR_TARGET;
        intent.targetEntityID = currentTargetID;
        intent.clearReason = MonsterAIClearReason::TARGET_INVALID;
        return intent;
    }

    if (monster.GetPosition().GetX() != monster.GetOriginalSpawnX() ||
        monster.GetPosition().GetY() != monster.GetOriginalSpawnY())
    {
        intent.type = MonsterAIIntentType::RETURN_HOME;
        intent.destination = Position(
            monster.GetOriginalSpawnX(),
            monster.GetOriginalSpawnY());
    }

    return intent;
}
