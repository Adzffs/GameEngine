#include "Monster.h"

#include "../../World/World.h"

Monster::Monster(
    int id,
    int x,
    int y,
    const CombatRatings &ratings,
    RewardTableType rewardTableType,
    std::optional<MonsterAggressionDefinition> aggressionDefinition,
    std::optional<MonsterRespawnDefinition> respawnDefinition,
    NpcType npcType,
    int attackDurationTicks,
    NpcSpawnId npcSpawnId,
    int wanderRadius,
    int wanderIntervalTicks)
    : Entity(id, EntityType::MONSTER),
      combatRatings(ratings),
      healthPool(ratings.maximumHealth),
      rewardTableType(rewardTableType),
      aggressionDefinition(aggressionDefinition),
      respawnDefinition(respawnDefinition),
      originalSpawnX(x),
      originalSpawnY(y),
      npcType(npcType),
      attackDurationTicks(attackDurationTicks),
      npcSpawnId(npcSpawnId),
      wanderRadius(wanderRadius),
      wanderIntervalTicks(wanderIntervalTicks)
{
    // Keep ratings and health cap internally consistent when health is clamped.
    combatRatings.maximumHealth = healthPool.GetMaximumHealth();

    GetPosition().SetPosition(x, y);
}

void Monster::Update(World &world)
{
    (void)world;
}

CombatRatings Monster::GetCombatRatings() const
{
    return combatRatings;
}

int Monster::GetCurrentHealth() const
{
    return healthPool.GetCurrentHealth();
}

int Monster::GetMaximumHealth() const
{
    return healthPool.GetMaximumHealth();
}

bool Monster::IsAlive() const
{
    return healthPool.IsAlive();
}

int Monster::ApplyDamage(int amount)
{
    return healthPool.ApplyDamage(amount);
}

void Monster::RestoreHealthToFull()
{
    healthPool.RestoreToFull();
}

RewardTableType Monster::GetRewardTableType() const
{
    return rewardTableType;
}

bool Monster::HasRespawnDefinition() const
{
    if (!respawnDefinition.has_value())
    {
        return false;
    }

    return respawnDefinition->delayTicks > 0;
}

std::optional<MonsterRespawnDefinition> Monster::GetRespawnDefinition() const
{
    return respawnDefinition;
}

bool Monster::HasAggressionDefinition() const
{
    return aggressionDefinition.has_value();
}

std::optional<MonsterAggressionDefinition> Monster::GetAggressionDefinition() const
{
    return aggressionDefinition;
}

int Monster::GetAggressionTargetEntityID() const
{
    return aggressionTargetEntityID;
}

void Monster::SetAggressionTargetEntityID(int targetEntityID)
{
    aggressionTargetEntityID = targetEntityID;
}

void Monster::ClearAggressionTargetEntityID()
{
    aggressionTargetEntityID = InvalidAggressionTargetEntityID;
}

int Monster::GetOriginalSpawnX() const
{
    return originalSpawnX;
}

int Monster::GetOriginalSpawnY() const
{
    return originalSpawnY;
}

NpcType Monster::GetNpcType() const
{
    return npcType;
}

int Monster::GetAttackDurationTicks() const
{
    return attackDurationTicks;
}

NpcSpawnId Monster::GetNpcSpawnId() const
{
    return npcSpawnId;
}

int Monster::GetWanderRadius() const
{
    return wanderRadius;
}

int Monster::GetWanderIntervalTicks() const
{
    return wanderIntervalTicks;
}
