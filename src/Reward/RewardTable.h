#pragma once

#include "GuaranteedRewardEntry.h"
#include "RewardTableValidationResult.h"
#include "WeightedRewardEntry.h"

#include <vector>

class RewardTable
{
public:
    RewardTableValidationResult AddGuaranteed(
        ItemType itemType,
        int minimumQuantity,
        int maximumQuantity);

    RewardTableValidationResult AddWeighted(
        ItemType itemType,
        int minimumQuantity,
        int maximumQuantity,
        int weight);

    const std::vector<GuaranteedRewardEntry> &
    GetGuaranteedEntries() const;

    const std::vector<WeightedRewardEntry> &
    GetWeightedEntries() const;

    int GetTotalWeight() const;

private:
    static RewardTableValidationResult ValidateItemType(
        ItemType itemType);

    static RewardTableValidationResult ValidateQuantityRange(
        int minimumQuantity,
        int maximumQuantity);

    std::vector<GuaranteedRewardEntry> guaranteedEntries;
    std::vector<WeightedRewardEntry> weightedEntries;

    int totalWeight = 0;
};
