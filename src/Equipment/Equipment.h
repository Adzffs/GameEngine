#pragma once

#include "EquipmentSlotType.h"
#include "../Inventory/ItemType.h"

#include <array>
#include <cstddef>

class Equipment
{
public:
    static constexpr std::size_t SlotCount =
        static_cast<std::size_t>(
            EquipmentSlotType::COUNT);

    Equipment();

    bool Equip(
        EquipmentSlotType slotType,
        ItemType itemType);

    ItemType Unequip(
        EquipmentSlotType slotType);

    ItemType GetEquippedItem(
        EquipmentSlotType slotType) const;

    bool IsSlotEmpty(
        EquipmentSlotType slotType) const;

    bool IsEquipped(
        ItemType itemType) const;

private:
    std::array<ItemType, SlotCount> equippedItems;
};