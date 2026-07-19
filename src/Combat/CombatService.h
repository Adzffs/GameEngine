#pragma once

#include "MeleeCompletionOutcome.h"
#include "MeleeCompletionContext.h"
#include "MeleeAttackResolver.h"

#include "../Action/Action.h"
#include "../Action/ActionValidationResult.h"
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

    MeleeCompletionOutcome EvaluateCompletedMeleeAction(
        const Action &action,
        const ActionValidationResult &validation,
        const MeleeCompletionContext &context) const;

    MeleeAttackResult EvaluateMeleeAttack(
        const CombatRatings &attacker,
        const CombatRatings &defender);

    int ApplyMeleeDamage(
        int rolledDamage,
        Combatant &defenderCombatant) const;

private:
    MeleeAttackResolver meleeAttackResolver;
    std::unique_ptr<RandomSource> randomSource;
};
