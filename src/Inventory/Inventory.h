#pragma once

#include "InventorySlot.h"

#include <array>

class Inventory
{
public:
    static constexpr int SlotCount = 28;

    bool AddItem(
        ItemType itemType,
        int amount);

    int GetItemAmount(
        ItemType itemType) const;

    bool HasItem(
        ItemType itemType) const;

    const std::array<
        InventorySlot,
        SlotCount> &
    GetSlots() const;

private:
    std::array<
        InventorySlot,
        SlotCount>
        slots;
};