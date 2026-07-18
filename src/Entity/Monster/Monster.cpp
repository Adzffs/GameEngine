#include "Monster.h"

#include "../../World/World.h"

Monster::Monster(
    int id,
    int x,
    int y,
    const CombatRatings &ratings)
    : Entity(id, EntityType::MONSTER),
      combatRatings(ratings),
      healthPool(ratings.maximumHealth)
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
