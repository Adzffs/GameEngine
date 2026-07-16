#pragma once

#include "ItemType.h"

class InventorySlot
{
public:
    InventorySlot();

    bool IsEmpty() const;

    ItemType GetItemType() const;
    int GetAmount() const;

    void SetItem(
        ItemType itemType,
        int amount);

    void AddAmount(int amount);

private:
    ItemType itemType;
    int amount;
};