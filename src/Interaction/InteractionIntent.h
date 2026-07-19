#pragma once

enum class InteractionTargetType
{
    RESOURCE,
    STATION
};

enum class InteractionIntentType
{
    READY,
    CLEAR_INTERACTION
};

enum class InteractionClearReason
{
    NONE,
    INVALID_ACTOR,
    ACTOR_DEAD,
    INVALID_TARGET,
    TARGET_TYPE_MISMATCH,
    APPROACH_FAILED,
    CANCELLED
};

struct InteractionIntent
{
    int actorEntityID;
    int targetObjectID;
    InteractionTargetType targetType;
    InteractionIntentType type;
    InteractionClearReason clearReason;
};
