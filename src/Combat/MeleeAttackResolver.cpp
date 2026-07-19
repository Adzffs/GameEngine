#include "MeleeAttackResolver.h"

#include "CombatFormulas.h"

MeleeAttackResult MeleeAttackResolver::Evaluate(
    const CombatRatings &attacker,
    const CombatRatings &defender,
    RandomSource &randomSource) const
{
    MeleeAttackResult result;

    result.attackerMaximumRoll =
        CombatFormulas::CalculateAttackRoll(attacker);

    result.defenderMaximumRoll =
        CombatFormulas::CalculateDefenceRoll(defender);

    result.maximumHit =
        CombatFormulas::CalculateMaximumHit(attacker);

    result.attackerRolledResult = randomSource.NextIntInclusive(
        0,
        result.attackerMaximumRoll);

    result.defenderRolledResult = randomSource.NextIntInclusive(
        0,
        result.defenderMaximumRoll);

    result.didHit = CombatFormulas::ResolveHit(
        result.attackerRolledResult,
        result.defenderRolledResult);

    if (!result.didHit)
    {
        return result;
    }

    int rawDamageRoll = randomSource.NextIntInclusive(
        0,
        result.maximumHit);

    result.rolledDamage = CombatFormulas::ClampDamageRoll(
        rawDamageRoll,
        result.maximumHit);

    return result;
}

int MeleeAttackResolver::ApplyRolledDamage(
    int rolledDamage,
    Combatant &defenderCombatant) const
{
    return defenderCombatant.ApplyDamage(
        rolledDamage);
}
