#pragma once

#include "MeleeAttackResult.h"

#include "../Core/RandomSource.h"
#include "../Stats/CombatRatings.h"
#include "../Stats/HealthPool.h"

class MeleeAttackResolver
{
public:
    MeleeAttackResult Resolve(
        const CombatRatings &attacker,
        const CombatRatings &defender,
        HealthPool &defenderHealth,
        RandomSource &randomSource) const;
};
