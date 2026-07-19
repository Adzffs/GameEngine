#pragma once

#include "MeleeCompletionOutcome.h"
#include "MeleeAttackResolver.h"

#include "../Action/Action.h"
#include "../Action/ActionValidationResult.h"
#include "Combatant.h"
#include "../Core/RandomSource.h"
#include "../Core/SeededRandom.h"
#include "../Stats/CombatRatings.h"

#include <functional>
#include <memory>
#include <optional>

class CombatService
{
public:
    explicit CombatService(unsigned int seed);

    explicit CombatService(std::unique_ptr<RandomSource> randomSource);

    using MeleeCompletionValidator =
        std::function<ActionValidationResult(
            int attackerEntityID,
            int defenderEntityID)>;

    using MeleeCombatRatingsProvider =
        std::function<std::optional<MeleeCombatRatingsSnapshot>(
            int attackerEntityID,
            int defenderEntityID)>;

    MeleeCompletionOutcome EvaluateCompletedMeleeAction(
        const Action &action,
        const MeleeCompletionValidator &completionValidator,
        const MeleeCombatRatingsProvider &ratingsProvider);

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
