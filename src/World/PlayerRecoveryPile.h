#pragma once

#include "../Inventory/ItemAmount.h"
#include "Position.h"

#include <vector>

struct PlayerRecoveryPile
{
    int ownerPlayerEntityID = -1;
    Position position{0, 0};
    std::vector<ItemAmount> items;
    int expiryTick = 0;
};
