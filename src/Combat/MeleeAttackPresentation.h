#pragma once

struct MeleeAttackPresentation
{
    int attackerEntityID = -1;
    int defenderEntityID = -1;
    int totalTicks = 1;
    int remainingTicks = 1;
};
