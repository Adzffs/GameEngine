#pragma once

#include "ItemType.h"

#include <map>

class Inventory
{
public:
    void AddItem(
        ItemType itemType,
        int amount);

    int GetItemAmount(
        ItemType itemType) const;

private:
    std::map<ItemType, int> items;
};