#pragma once

struct MeleeAttackResult
{
    int attackerMaximumRoll = 0;
    int defenderMaximumRoll = 0;

    int attackerRolledResult = 0;
    int defenderRolledResult = 0;

    int maximumHit = 0;
    bool didHit = false;

    int rolledDamage = 0;
    int actualDamageApplied = 0;
};
