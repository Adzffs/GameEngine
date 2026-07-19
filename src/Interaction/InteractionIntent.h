#pragma once
#include "../NPC/NpcInteractionType.h"

enum class InteractionTargetType
{
    RESOURCE,
    STATION,
    NPC
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
    CANCELLED,
    UNSUPPORTED_INTERACTION
};

struct InteractionIntent
{
    int actorEntityID;
    int targetObjectID;
    InteractionTargetType targetType;
    InteractionIntentType type;
    InteractionClearReason clearReason;
    NpcInteractionType npcInteractionType = NpcInteractionType::TALK;
};
