#pragma once

#include "../Stats/CombatRatings.h"

class Combatant
{
public:
    virtual ~Combatant() = default;

    virtual CombatRatings GetCombatRatings() const = 0;

    virtual int GetCurrentHealth() const = 0;
    virtual int GetMaximumHealth() const = 0;
    virtual bool IsAlive() const = 0;

    virtual int ApplyDamage(int amount) = 0;
};