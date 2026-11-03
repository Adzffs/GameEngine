#pragma once

#include "ItemTag.h"
#include "../Inventory/ItemType.h"

class ItemTagDatabase
{
public:
    static bool IsValid(ItemTag tag);
    static bool Contains(ItemTag tag, ItemType itemType);
};
