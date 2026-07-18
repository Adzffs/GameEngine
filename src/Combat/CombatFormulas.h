#pragma once

#include "../Stats/CombatRatings.h"

namespace CombatFormulas
{
    struct MeleeCombatProfile
    {
        int attackRoll = 0;
        int defenceRoll = 0;
        int maximumHit = 0;
    };

    int CalculateAttackRoll(
        const CombatRatings &attacker);

    int CalculateDefenceRoll(
        const CombatRatings &defender);

    int CalculateMaximumHit(
        const CombatRatings &attacker);

    double CalculateHitChance(
        int attackRoll,
        int defenceRoll);

    double CalculateHitChance(
        const CombatRatings &attacker,
        const CombatRatings &defender);

    bool ResolveHit(
        int attackerRollResult,
        int defenderRollResult);

    int ClampDamageRoll(
        int rolledDamage,
        int maximumHit);

    MeleeCombatProfile BuildMeleeCombatProfile(
        const CombatRatings &ratings);
}