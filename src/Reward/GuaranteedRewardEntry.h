#pragma once

#include "../Inventory/ItemType.h"

struct GuaranteedRewardEntry
{
    ItemType itemType;
    int minimumQuantity;
    int maximumQuantity;
};
