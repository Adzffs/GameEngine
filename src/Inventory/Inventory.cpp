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

bool Inventory::CanAddItem(
    ItemType itemType,
    int amount) const
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
        for (const InventorySlot &slot : slots)
        {
            if (!slot.IsEmpty() &&
                slot.GetItemType() == itemType)
            {
                return true;
            }
        }

        for (const InventorySlot &slot : slots)
        {
            if (slot.IsEmpty())
            {
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

    return emptySlotCount >= amount;
}

bool Inventory::RemoveItem(
    ItemType itemType,
    int amount)
{
    if (itemType == ItemType::NONE ||
        amount <= 0)
    {
        return false;
    }

    if (GetItemAmount(itemType) < amount)
    {
        return false;
    }

    int remainingAmount = amount;

    for (InventorySlot &slot : slots)
    {
        if (slot.IsEmpty() ||
            slot.GetItemType() != itemType)
        {
            continue;
        }

        int amountToRemove =
            slot.GetAmount();

        if (amountToRemove >
            remainingAmount)
        {
            amountToRemove =
                remainingAmount;
        }

        slot.RemoveAmount(
            amountToRemove);

        remainingAmount -=
            amountToRemove;

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