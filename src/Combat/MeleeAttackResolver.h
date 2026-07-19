#pragma once

#include "MeleeAttackResult.h"

#include "Combatant.h"
#include "../Core/RandomSource.h"
#include "../Stats/CombatRatings.h"

class MeleeAttackResolver
{
public:
    MeleeAttackResult Evaluate(
        const CombatRatings &attacker,
        const CombatRatings &defenderRatings,
        RandomSource &randomSource) const;

    int ApplyRolledDamage(
        int rolledDamage,
        Combatant &defenderCombatant) const;
};
