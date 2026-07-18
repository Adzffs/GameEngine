#include "RewardTableRoller.h"

#include <limits>

RewardTableRoller::RewardTableRoller(
    RandomSource &randomSource)
    : randomSource(randomSource)
{
}

std::vector<ItemReward> RewardTableRoller::Roll(
    const RewardTable &table)
{
    std::vector<ItemReward> rewards;

    for (const GuaranteedRewardEntry &entry :
         table.GetGuaranteedEntries())
    {
        int quantity = randomSource.NextIntInclusive(
            entry.minimumQuantity,
            entry.maximumQuantity);

        if (quantity < 1)
        {
            return {};
        }

        if (!AppendOrMergeReward(
                rewards,
                entry.itemType,
                quantity))
        {
            return {};
        }
    }

    const std::vector<WeightedRewardEntry> &weightedEntries =
        table.GetWeightedEntries();

    if (weightedEntries.empty())
    {
        return rewards;
    }

    int totalWeight = table.GetTotalWeight();

    if (totalWeight <= 0)
    {
        return {};
    }

    int selectedWeight = randomSource.NextIntInclusive(
        1,
        totalWeight);

    const WeightedRewardEntry *selectedEntry = nullptr;
    int runningWeight = 0;

    for (const WeightedRewardEntry &entry : weightedEntries)
    {
        if (entry.weight <= 0)
        {
            return {};
        }

        if (runningWeight >
            std::numeric_limits<int>::max() - entry.weight)
        {
            return {};
        }

        runningWeight += entry.weight;

        if (selectedEntry == nullptr &&
            selectedWeight <= runningWeight)
        {
            selectedEntry = &entry;
        }
    }

    if (selectedEntry == nullptr)
    {
        return {};
    }

    int selectedQuantity = randomSource.NextIntInclusive(
        selectedEntry->minimumQuantity,
        selectedEntry->maximumQuantity);

    if (selectedQuantity < 1)
    {
        return {};
    }

    if (!AppendOrMergeReward(
            rewards,
            selectedEntry->itemType,
            selectedQuantity))
    {
        return {};
    }

    return rewards;
}

bool RewardTableRoller::AppendOrMergeReward(
    std::vector<ItemReward> &rewards,
    ItemType itemType,
    int quantity)
{
    if (quantity < 1)
    {
        return false;
    }

    for (ItemReward &reward : rewards)
    {
        if (reward.itemType == itemType)
        {
            if (reward.quantity >
                std::numeric_limits<int>::max() - quantity)
            {
                return false;
            }

            reward.quantity += quantity;
            return true;
        }
    }

    rewards.push_back(
        ItemReward{
            itemType,
            quantity});

    return true;
}
