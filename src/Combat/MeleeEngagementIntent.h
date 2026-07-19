#pragma once

enum class MeleeEngagementIntentType
{
    NONE,
    APPROACH_TARGET,
    ATTACK_TARGET,
    CLEAR_ENGAGEMENT
};

enum class MeleeEngagementClearReason
{
    NONE,
    INVALID_ATTACKER,
    INVALID_TARGET,
    ATTACKER_DEAD,
    TARGET_DEAD,
    CANCELLED
};

struct MeleeEngagementIntent
{
    int attackerEntityID;
    int targetEntityID;
    int durationTicks;
    MeleeEngagementIntentType type;
    MeleeEngagementClearReason clearReason;
};
