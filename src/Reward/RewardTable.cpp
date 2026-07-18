#include "RewardTable.h"

#include "../Item/ItemDatabase.h"

#include <limits>

RewardTableValidationResult RewardTable::AddGuaranteed(
    ItemType itemType,
    int minimumQuantity,
    int maximumQuantity)
{
    RewardTableValidationResult itemValidation =
        ValidateItemType(itemType);

    if (!itemValidation.valid)
    {
        return itemValidation;
    }

    RewardTableValidationResult quantityValidation =
        ValidateQuantityRange(
            minimumQuantity,
            maximumQuantity);

    if (!quantityValidation.valid)
    {
        return quantityValidation;
    }

    guaranteedEntries.push_back(
        GuaranteedRewardEntry{
            itemType,
            minimumQuantity,
            maximumQuantity});

    return {
        true,
        ""};
}

RewardTableValidationResult RewardTable::AddWeighted(
    ItemType itemType,
    int minimumQuantity,
    int maximumQuantity,
    int weight)
{
    RewardTableValidationResult itemValidation =
        ValidateItemType(itemType);

    if (!itemValidation.valid)
    {
        return itemValidation;
    }

    RewardTableValidationResult quantityValidation =
        ValidateQuantityRange(
            minimumQuantity,
            maximumQuantity);

    if (!quantityValidation.valid)
    {
        return quantityValidation;
    }

    if (weight <= 0)
    {
        return {
            false,
            "Reward entry weight must be positive"};
    }

    if (totalWeight >
        std::numeric_limits<int>::max() - weight)
    {
        return {
            false,
            "Total reward-table weight overflow"};
    }

    weightedEntries.push_back(
        WeightedRewardEntry{
            itemType,
            minimumQuantity,
            maximumQuantity,
            weight});

    totalWeight += weight;

    return {
        true,
        ""};
}

const std::vector<GuaranteedRewardEntry> &
RewardTable::GetGuaranteedEntries() const
{
    return guaranteedEntries;
}

const std::vector<WeightedRewardEntry> &
RewardTable::GetWeightedEntries() const
{
    return weightedEntries;
}

int RewardTable::GetTotalWeight() const
{
    return totalWeight;
}

RewardTableValidationResult RewardTable::ValidateItemType(
    ItemType itemType)
{
    if (itemType == ItemType::NONE)
    {
        return {
            false,
            "Reward entry item type must be valid"};
    }

    const ItemDefinition &definition =
        ItemDatabase::Get(itemType);

    if (definition.GetItemType() != itemType)
    {
        return {
            false,
            "Reward entry item type must be valid"};
    }

    return {
        true,
        ""};
}

RewardTableValidationResult RewardTable::ValidateQuantityRange(
    int minimumQuantity,
    int maximumQuantity)
{
    if (minimumQuantity < 1)
    {
        return {
            false,
            "Reward minimum quantity must be at least one"};
    }

    if (maximumQuantity < minimumQuantity)
    {
        return {
            false,
            "Reward maximum quantity must be greater than or equal to minimum"};
    }

    return {
        true,
        ""};
}
