#pragma once

#include "../Inventory/ItemType.h"

struct MeleeCombatFeedback
{
    int attackerEntityID = -1;
    int defenderEntityID = -1;
    bool hit = false;
    int actualDamageApplied = 0;
    int remainingTicks = 0;
    // Result identity and geometry survive a killing blow/removal of its target.
    int completionTick = -1;
    int attackerX = 0, attackerY = 0;
    int defenderX = 0, defenderY = 0;
    ItemType weapon = ItemType::NONE;
};
