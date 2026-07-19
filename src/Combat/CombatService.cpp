#include "CombatService.h"

#include "../Action/ActionType.h"

#include <stdexcept>
#include <utility>

CombatService::CombatService(unsigned int seed)
    : randomSource(std::make_unique<SeededRandom>(seed))
{
}

CombatService::CombatService(std::unique_ptr<RandomSource> randomSource)
    : randomSource(std::move(randomSource))
{
    if (!this->randomSource)
    {
        throw std::invalid_argument("CombatService requires a non-null RandomSource");
    }
}

MeleeCompletionOutcome CombatService::EvaluateCompletedMeleeAction(
    const Action &action,
    const MeleeCompletionValidator &completionValidator,
    const MeleeCombatRatingsProvider &ratingsProvider)
{
    MeleeCompletionOutcome outcome;
    outcome.attackerEntityID =
        action.GetOwnerID();
    outcome.defenderEntityID =
        action.GetTargetID();

    if (action.GetType() !=
        ActionType::MELEE_ATTACK)
    {
        outcome.type =
            MeleeCompletionOutcomeType::IGNORE;
        return outcome;
    }

    ActionValidationResult validation =
        completionValidator(
            action.GetOwnerID(),
            action.GetTargetID());

    if (!validation.valid)
    {
        outcome.type =
            MeleeCompletionOutcomeType::CANCEL;
        outcome.cancelReason =
            validation.reason;
        outcome.message =
            validation.message;
        return outcome;
    }

    std::optional<MeleeCombatRatingsSnapshot> ratingsSnapshot =
        ratingsProvider(
            action.GetOwnerID(),
            action.GetTargetID());

    if (!ratingsSnapshot.has_value())
    {
        outcome.type =
            MeleeCompletionOutcomeType::CLEAR_STALE;
        return outcome;
    }

    outcome.type =
        MeleeCompletionOutcomeType::RESOLVE;
    outcome.ratingsSnapshot =
        ratingsSnapshot;

    return outcome;
}

MeleeAttackResult CombatService::EvaluateMeleeAttack(
    const CombatRatings &attacker,
    const CombatRatings &defender)
{
    return meleeAttackResolver.Evaluate(
        attacker,
        defender,
        *randomSource);
}

int CombatService::ApplyMeleeDamage(
    int rolledDamage,
    Combatant &defenderCombatant) const
{
    return meleeAttackResolver.ApplyRolledDamage(
        rolledDamage,
        defenderCombatant);
}
