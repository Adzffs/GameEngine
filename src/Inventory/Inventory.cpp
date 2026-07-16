#include "Inventory.h"

void Inventory::AddItem(
    ItemType itemType,
    int amount)
{
    if (amount <= 0)
    {
        return;
    }

    items[itemType] += amount;
}

int Inventory::GetItemAmount(
    ItemType itemType) const
{
    auto itemIterator =
        items.find(itemType);

    if (itemIterator == items.end())
    {
        return 0;
    }

    return itemIterator->second;
}