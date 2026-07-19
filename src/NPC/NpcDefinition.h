#pragma once

#include "NpcType.h"
#include "NpcKind.h"
#include "NpcInteractionType.h"
#include "../Dialogue/DialogueId.h"
#include "../Shop/ShopId.h"
#include "../Entity/Monster/MonsterAggressionDefinition.h"
#include "../Reward/RewardTableType.h"
#include "../Stats/CombatRatings.h"

#include <optional>
#include <string>
#include <vector>

struct NpcCombatDefinition
{
    CombatRatings ratings;
    int attackDurationTicks;
    RewardTableType rewardTableType;
    std::optional<MonsterAggressionDefinition> aggression;
    int respawnDelayTicks;
};

struct NpcDefinition
{
    NpcType type = NpcType::NONE;
    std::string name;
    NpcKind kind = NpcKind::FRIENDLY;
    std::optional<NpcCombatDefinition> combat;
    std::vector<NpcInteractionType> interactions;
    DialogueId dialogueId = DialogueId::NONE;
    ShopId shopId = ShopId::NONE;
};
