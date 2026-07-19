#pragma once

#include "../Action/ActionCancelReason.h"
#include "../Stats/CombatRatings.h"

#include <optional>
#include <string>

enum class MeleeCompletionOutcomeType
{
    IGNORE,
    CLEAR_STALE,
    CANCEL,
    RESOLVE
};

struct MeleeCombatRatingsSnapshot
{
    CombatRatings attackerRatings;
    CombatRatings defenderRatings;
};

struct MeleeCompletionOutcome
{
    MeleeCompletionOutcomeType type =
        MeleeCompletionOutcomeType::IGNORE;

    int attackerEntityID = 0;
    int defenderEntityID = 0;

    ActionCancelReason cancelReason =
        ActionCancelReason::NONE;

    std::string message;

    std::optional<MeleeCombatRatingsSnapshot>
        ratingsSnapshot;
};