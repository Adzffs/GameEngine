#include "InventorySlot.h"

InventorySlot::InventorySlot()
    : itemType(ItemType::NONE),
      amount(0)
{
}

bool InventorySlot::IsEmpty() const
{
    return itemType == ItemType::NONE;
}

ItemType InventorySlot::GetItemType() const
{
    return itemType;
}

int InventorySlot::GetAmount() const
{
    return amount;
}

void InventorySlot::SetItem(
    ItemType newItemType,
    int newAmount)
{
    itemType = newItemType;
    amount = newAmount;
}

void InventorySlot::AddAmount(int addedAmount)
{
    amount += addedAmount;
}