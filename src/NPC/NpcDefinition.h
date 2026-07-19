#pragma once

#include "NpcType.h"
#include "../Entity/Monster/MonsterAggressionDefinition.h"
#include "../Reward/RewardTableType.h"
#include "../Stats/CombatRatings.h"

#include <optional>
#include <string>

struct NpcDefinition
{
    NpcType type = NpcType::NONE;
    std::string name;
    CombatRatings combatRatings;
    std::optional<MonsterAggressionDefinition> aggressionDefinition;
    int attackDurationTicks = 0;
    int respawnDelayTicks = 0;
    RewardTableType rewardTableType = RewardTableType::NONE;
};
