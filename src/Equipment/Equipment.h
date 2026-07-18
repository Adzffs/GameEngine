#pragma once

#include "EquipmentSlotType.h"
#include "../Inventory/ItemType.h"
#include "../Stats/StatBlock.h"

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

    StatBlock GetTotalStatBonuses() const;

private:
    std::array<ItemType, SlotCount> equippedItems;
};