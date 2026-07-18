#pragma once

#include "MeleeAttackResult.h"

#include "Combatant.h"
#include "../Core/RandomSource.h"
#include "../Stats/CombatRatings.h"

class MeleeAttackResolver
{
public:
    MeleeAttackResult Resolve(
        const CombatRatings &attacker,
        const CombatRatings &defenderRatings,
        Combatant &defenderCombatant,
        RandomSource &randomSource) const;
};
