#pragma once

#include "../Inventory/ItemType.h"

struct WeightedRewardEntry
{
    ItemType itemType;
    int minimumQuantity;
    int maximumQuantity;
    int weight;
};
