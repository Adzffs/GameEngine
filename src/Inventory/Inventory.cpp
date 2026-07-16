#include "Inventory.h"

#include "../Item/ItemDatabase.h"

bool Inventory::AddItem(
    ItemType itemType,
    int amount)
{
    if (itemType == ItemType::NONE ||
        amount <= 0)
    {
        return false;
    }

    const ItemDefinition &definition =
        ItemDatabase::Get(itemType);

    if (definition.IsStackable())
    {
        for (InventorySlot &slot : slots)
        {
            if (!slot.IsEmpty() &&
                slot.GetItemType() == itemType)
            {
                slot.AddAmount(amount);
                return true;
            }
        }

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

    int emptySlotCount = 0;

    for (const InventorySlot &slot : slots)
    {
        if (slot.IsEmpty())
        {
            emptySlotCount++;
        }
    }

    // Prevent partially adding an item request.
    if (emptySlotCount < amount)
    {
        return false;
    }

    int remainingAmount = amount;

    for (InventorySlot &slot : slots)
    {
        if (!slot.IsEmpty())
        {
            continue;
        }

        slot.SetItem(
            itemType,
            1);

        remainingAmount--;

        if (remainingAmount == 0)
        {
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

bool Inventory::HasItem(
    ItemType itemType) const
{
    return GetItemAmount(itemType) > 0;
}

const std::array<
    InventorySlot,
    Inventory::SlotCount> &
Inventory::GetSlots() const
{
    return slots;
}