#include "Inventory.h"

#include "../Item/ItemDatabase.h"

#include <limits>
#include <algorithm>

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

bool Inventory::RemoveItemFromSlot(
    int slotIndex,
    int amount)
{
    if (slotIndex < 0 ||
        slotIndex >= SlotCount ||
        amount <= 0)
    {
        return false;
    }

    InventorySlot &slot =
        slots[slotIndex];

    if (slot.IsEmpty() ||
        slot.GetAmount() < amount)
    {
        return false;
    }

    slot.RemoveAmount(amount);
    return true;
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

bool Inventory::TryReplaceSlotsAtomically(
    const std::array<
        InventorySlot,
        SlotCount> &replacementSlots)
{
    std::vector<ItemType> stackableItems;

    for (const InventorySlot &slot : replacementSlots)
    {
        ItemType itemType = slot.GetItemType();
        int quantity = slot.GetAmount();

        if (itemType == ItemType::NONE)
        {
            if (quantity != 0)
            {
                return false;
            }
            continue;
        }

        const std::vector<ItemType> &knownItems =
            ItemDatabase::GetAllItemTypes();
        if (std::find(
                knownItems.begin(),
                knownItems.end(),
                itemType) == knownItems.end() ||
            quantity <= 0)
        {
            return false;
        }

        const ItemDefinition &definition =
            ItemDatabase::Get(itemType);
        if (!definition.IsStackable())
        {
            if (quantity != 1)
            {
                return false;
            }
            continue;
        }

        if (std::find(
                stackableItems.begin(),
                stackableItems.end(),
                itemType) != stackableItems.end())
        {
            return false;
        }
        stackableItems.push_back(itemType);
    }

    slots = replacementSlots;
    return true;
}

bool Inventory::TryAddItemsAtomically(
    const std::vector<ItemAmount> &items)
{
    Inventory simulated = *this;

    for (const ItemAmount &item : items)
    {
        if (item.itemType == ItemType::NONE ||
            item.quantity <= 0)
        {
            return false;
        }

        const ItemDefinition &definition =
            ItemDatabase::Get(item.itemType);

        if (definition.GetItemType() != item.itemType)
        {
            return false;
        }

        if (definition.IsStackable())
        {
            int currentAmount =
                simulated.GetItemAmount(
                    item.itemType);

            if (currentAmount >
                std::numeric_limits<int>::max() - item.quantity)
            {
                return false;
            }
        }

        if (!simulated.AddItem(
                item.itemType,
                item.quantity))
        {
            return false;
        }
    }

    *this = simulated;
    return true;
}

bool Inventory::TryApplyTransactionAtomically(
    const std::vector<ItemAmount> &removals,
    const std::vector<ItemAmount> &additions)
{
    Inventory simulated = *this;
    for (const ItemAmount &item : removals)
    {
        if (item.itemType == ItemType::NONE || item.quantity <= 0 ||
            ItemDatabase::Get(item.itemType).GetItemType() != item.itemType ||
            !simulated.RemoveItem(item.itemType, item.quantity))
            return false;
    }
    for (const ItemAmount &item : additions)
    {
        if (item.itemType == ItemType::NONE || item.quantity <= 0 ||
            ItemDatabase::Get(item.itemType).GetItemType() != item.itemType)
            return false;
        if (ItemDatabase::Get(item.itemType).IsStackable() &&
            simulated.GetItemAmount(item.itemType) >
                std::numeric_limits<int>::max() - item.quantity)
            return false;
        if (!simulated.AddItem(item.itemType, item.quantity))
            return false;
    }
    *this = simulated;
    return true;
}
