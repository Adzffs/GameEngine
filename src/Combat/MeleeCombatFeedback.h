#pragma once

struct MeleeCombatFeedback
{
    int attackerEntityID = -1;
    int defenderEntityID = -1;
    bool hit = false;
    int actualDamageApplied = 0;
    int remainingTicks = 0;
};