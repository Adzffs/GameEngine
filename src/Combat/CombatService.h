#pragma once

#include "MeleeAttackResolver.h"

#include "../Core/RandomSource.h"
#include "../Core/SeededRandom.h"
#include "../Stats/CombatRatings.h"
#include "../Stats/HealthPool.h"

#include <memory>

class CombatService
{
public:
    explicit CombatService(unsigned int seed);

    explicit CombatService(std::unique_ptr<RandomSource> randomSource);

    MeleeAttackResult ResolveMeleeAttack(
        const CombatRatings &attacker,
        const CombatRatings &defender,
        HealthPool &defenderHealth);

private:
    MeleeAttackResolver meleeAttackResolver;
    std::unique_ptr<RandomSource> randomSource;
};
