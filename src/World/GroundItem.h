#pragma once

#include "../Inventory/ItemType.h"
#include "Position.h"

struct GroundItem
{
    int id = -1;
    ItemType itemType = ItemType::NONE;
    int amount = 0;
    Position position;
};
