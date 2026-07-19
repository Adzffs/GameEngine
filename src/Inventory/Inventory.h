#pragma once

#include "ItemAmount.h"
#include "InventorySlot.h"

#include <array>
#include <vector>

class Inventory
{
public:
    static constexpr int SlotCount = 28;

    bool AddItem(
        ItemType itemType,
        int amount);

    bool CanAddItem(
        ItemType itemType,
        int amount) const;

    bool RemoveItem(
        ItemType itemType,
        int amount);

    bool RemoveItemFromSlot(
        int slotIndex,
        int amount);

    int GetItemAmount(
        ItemType itemType) const;

    bool HasItem(
        ItemType itemType) const;

    const std::array<
        InventorySlot,
        SlotCount> &
    GetSlots() const;

    bool TryAddItemsAtomically(
        const std::vector<ItemAmount> &items);

    // Items are value types today; there is no durability or unique-instance
    // metadata, so type-and-quantity transactions are authoritative.
    bool TryApplyTransactionAtomically(
        const std::vector<ItemAmount> &removals,
        const std::vector<ItemAmount> &additions);

private:
    std::array<
        InventorySlot,
        SlotCount>
        slots;
};
