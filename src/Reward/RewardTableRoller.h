#pragma once

#include "ItemReward.h"
#include "RewardTable.h"

#include "../Core/RandomSource.h"

#include <vector>

class RewardTableRoller
{
public:
    explicit RewardTableRoller(
        RandomSource &randomSource);

    // Duplicate item rewards are merged by ItemType in first-seen order.
    std::vector<ItemReward> Roll(
        const RewardTable &table);

private:
    RandomSource &randomSource;

    static bool AppendOrMergeReward(
        std::vector<ItemReward> &rewards,
        ItemType itemType,
        int quantity);
};
