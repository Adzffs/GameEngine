#pragma once

#include "ItemDefinition.h"

#include <vector>

class ItemDatabase
{
public:
    static const ItemDefinition &Get(
        ItemType itemType);

    static const std::vector<ItemType> &
    GetAllItemTypes();
};