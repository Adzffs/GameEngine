#include "CombatFormulas.h"

#include <algorithm>
#include <cmath>

namespace CombatFormulas
{
    namespace
    {
        int ClampAtLeastOne(int value)
        {
            return std::max(1, value);
        }

        double ClampChance(double value)
        {
            return std::clamp(
                value,
                0.0,
                1.0);
        }
    }

    int CalculateAttackRoll(
        const CombatRatings &attacker)
    {
        return ClampAtLeastOne(
            attacker.attackAccuracy);
    }

    int CalculateDefenceRoll(
        const CombatRatings &defender)
    {
        return ClampAtLeastOne(
            defender.defence);
    }

    int CalculateMaximumHit(
        const CombatRatings &attacker)
    {
        double scaledStrength =
            std::floor(
                static_cast<double>(attacker.meleeStrength) / 10.0);

        int maximumHit =
            1 + static_cast<int>(scaledStrength);

        return ClampAtLeastOne(maximumHit);
    }

    double CalculateHitChance(
        int attackRoll,
        int defenceRoll)
    {
        int clampedAttackRoll = ClampAtLeastOne(attackRoll);
        int clampedDefenceRoll = ClampAtLeastOne(defenceRoll);

        double chance = 0.0;

        if (clampedAttackRoll > clampedDefenceRoll)
        {
            chance = 1.0 -
                     ((static_cast<double>(clampedDefenceRoll) + 2.0) /
                      (2.0 * (static_cast<double>(clampedAttackRoll) + 1.0)));
        }
        else
        {
            chance = static_cast<double>(clampedAttackRoll) /
                     (2.0 * (static_cast<double>(clampedDefenceRoll) + 1.0));
        }

        return ClampChance(chance);
    }

    double CalculateHitChance(
        const CombatRatings &attacker,
        const CombatRatings &defender)
    {
        return CalculateHitChance(
            CalculateAttackRoll(attacker),
            CalculateDefenceRoll(defender));
    }

    bool ResolveHit(
        int attackerRollResult,
        int defenderRollResult)
    {
        return attackerRollResult > defenderRollResult;
    }

    int ClampDamageRoll(
        int rolledDamage,
        int maximumHit)
    {
        int clampedMaximumHit = ClampAtLeastOne(maximumHit);

        if (rolledDamage < 0)
        {
            return 0;
        }

        return std::min(
            rolledDamage,
            clampedMaximumHit);
    }

    MeleeCombatProfile BuildMeleeCombatProfile(
        const CombatRatings &ratings)
    {
        MeleeCombatProfile profile;
        profile.attackRoll = CalculateAttackRoll(ratings);
        profile.defenceRoll = CalculateDefenceRoll(ratings);
        profile.maximumHit = CalculateMaximumHit(ratings);
        return profile;
    }
}