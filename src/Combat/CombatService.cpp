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
    const ActionValidationResult &validation,
    const MeleeCompletionContext &context) const
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

    if (context.status !=
        MeleeCompletionContextStatus::VALID)
    {
        outcome.type =
            MeleeCompletionOutcomeType::CLEAR_STALE;
        outcome.staleReason = context.status;
        return outcome;
    }

    outcome.type =
        MeleeCompletionOutcomeType::RESOLVE;
    outcome.attackerEntityID =
        context.attackerEntityID;
    outcome.defenderEntityID =
        context.defenderEntityID;
    outcome.attackerRatings =
        context.attackerRatings;
    outcome.defenderRatings =
        context.defenderRatings;

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
