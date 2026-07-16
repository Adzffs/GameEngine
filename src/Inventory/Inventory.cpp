#include "Inventory.h"

bool Inventory::AddItem(
    ItemType itemType,
    int amount)
{
    if (itemType == ItemType::NONE ||
        amount <= 0)
    {
        return false;
    }

    // Stack the item if it already exists.
    for (InventorySlot &slot : slots)
    {
        if (!slot.IsEmpty() &&
            slot.GetItemType() == itemType)
        {
            slot.AddAmount(amount);
            return true;
        }
    }

    // Otherwise use the first empty slot.
    for (InventorySlot &slot : slots)
    {
        if (slot.IsEmpty())
        {
            slot.SetItem(
                itemType,
                amount);

            return true;
        }
    }

    return false;
}

int Inventory::GetItemAmount(
    ItemType itemType) const
{
    int totalAmount = 0;

    for (const InventorySlot &slot : slots)
    {
        if (!slot.IsEmpty() &&
            slot.GetItemType() == itemType)
        {
            totalAmount += slot.GetAmount();
        }
    }

    return totalAmount;
}

const std::array<
    InventorySlot,
    Inventory::SlotCount> &
Inventory::GetSlots() const
{
    return slots;
}