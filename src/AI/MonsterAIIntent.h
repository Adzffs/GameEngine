#pragma once

#include "../World/Position.h"

enum class MonsterAIIntentType
{
    NONE,
    CLEAR_TARGET,
    CHASE_TARGET,
    ATTACK_TARGET,
    RETURN_HOME
};

enum class MonsterAIClearReason
{
    NONE,
    TARGET_INVALID,
    LEASH_VIOLATED
};

struct MonsterAIIntent
{
    MonsterAIIntentType type = MonsterAIIntentType::NONE;
    int monsterEntityID = 0;
    int targetEntityID = -1;
    Position destination = Position(0, 0);
    MonsterAIClearReason clearReason = MonsterAIClearReason::NONE;
};
