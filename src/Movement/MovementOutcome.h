#pragma once

#include "../World/Position.h"

enum class MovementOutcomeType
{
    STEP_SUCCEEDED,
    ARRIVED,
    PATH_RECALCULATED,
    PATH_CANCELLED
};

struct MovementOutcome
{
    int entityID;
    MovementOutcomeType type;
    Position previousPosition;
    Position currentPosition;
    Position requestedDestination;
};
