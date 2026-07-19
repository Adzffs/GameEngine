#pragma once

#include "RewardTable.h"
#include "RewardTableType.h"

#include <vector>

namespace RewardTableRegistry
{
    const std::vector<RewardTableType> &
    GetAllRewardTableTypes();

    const RewardTable *TryGetRewardTable(
        RewardTableType rewardTableType);
}
