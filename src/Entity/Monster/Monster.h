#pragma once

#include "../../Combat/Combatant.h"
#include "../Entity.h"
#include "../../Stats/HealthPool.h"

class Monster : public Entity, public Combatant
{
public:
    Monster(
        int id,
        int x,
        int y,
        const CombatRatings &ratings);

    void Update(World &world) override;

    CombatRatings GetCombatRatings() const override;

    int GetCurrentHealth() const override;
    int GetMaximumHealth() const override;
    bool IsAlive() const override;

    int ApplyDamage(int amount) override;

private:
    CombatRatings combatRatings;
    HealthPool healthPool;
};