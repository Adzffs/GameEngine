#pragma once

#include "MeleeAttackResolver.h"

#include "Combatant.h"
#include "../Core/RandomSource.h"
#include "../Core/SeededRandom.h"
#include "../Stats/CombatRatings.h"

#include <memory>

class CombatService
{
public:
    explicit CombatService(unsigned int seed);

    explicit CombatService(std::unique_ptr<RandomSource> randomSource);

    MeleeAttackResult ResolveMeleeAttack(
        const CombatRatings &attacker,
        const CombatRatings &defender,
        Combatant &defenderCombatant);

private:
    MeleeAttackResolver meleeAttackResolver;
    std::unique_ptr<RandomSource> randomSource;
};
