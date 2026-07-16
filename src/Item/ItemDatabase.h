#pragma once

#include "ItemDefinition.h"

class ItemDatabase
{
public:
    static const ItemDefinition &Get(
        ItemType itemType);
};