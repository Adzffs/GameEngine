#pragma once

#include "../Action/ActionCancelReason.h"
#include "MeleeCompletionContext.h"
#include "../Stats/CombatRatings.h"

#include <string>

enum class MeleeCompletionOutcomeType
{
    IGNORE,
    CLEAR_STALE,
    CANCEL,
    RESOLVE
};

struct MeleeCompletionOutcome
{
    MeleeCompletionOutcomeType type =
        MeleeCompletionOutcomeType::IGNORE;

    int attackerEntityID = 0;
    int defenderEntityID = 0;

    ActionCancelReason cancelReason =
        ActionCancelReason::NONE;

    MeleeCompletionContextStatus staleReason =
        MeleeCompletionContextStatus::VALID;

    std::string message;

    CombatRatings attackerRatings;
    CombatRatings defenderRatings;
};
