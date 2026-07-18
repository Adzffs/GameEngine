#pragma once

#include "RewardTable.h"
#include "RewardTableType.h"

namespace RewardTableRegistry
{
    const RewardTable *TryGetRewardTable(
        RewardTableType rewardTableType);
}
