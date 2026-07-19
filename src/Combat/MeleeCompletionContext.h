#pragma once

#include "../Stats/CombatRatings.h"

enum class MeleeCompletionContextStatus
{
    VALID,
    MISSING_ATTACKER,
    MISSING_DEFENDER,
    INVALID_ATTACKER,
    INVALID_DEFENDER
};

struct MeleeCompletionContext
{
    MeleeCompletionContextStatus status =
        MeleeCompletionContextStatus::MISSING_ATTACKER;

    int attackerEntityID = 0;
    int defenderEntityID = 0;

    CombatRatings attackerRatings;
    CombatRatings defenderRatings;
};
