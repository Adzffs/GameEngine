#include "Equipment.h"

Equipment::Equipment()
{
    equippedItems.fill(ItemType::NONE);
}

bool Equipment::Equip(
    EquipmentSlotType slotType,
    ItemType itemType)
{
    if (itemType == ItemType::NONE)
    {
        return false;
    }

    std::size_t slotIndex =
        static_cast<std::size_t>(slotType);

    if (slotIndex >= equippedItems.size())
    {
        return false;
    }

    if (equippedItems[slotIndex] !=
        ItemType::NONE)
    {
        return false;
    }

    equippedItems[slotIndex] = itemType;
    return true;
}

ItemType Equipment::Unequip(
    EquipmentSlotType slotType)
{
    std::size_t slotIndex =
        static_cast<std::size_t>(slotType);

    if (slotIndex >= equippedItems.size())
    {
        return ItemType::NONE;
    }

    ItemType removedItem =
        equippedItems[slotIndex];

    equippedItems[slotIndex] =
        ItemType::NONE;

    return removedItem;
}

ItemType Equipment::GetEquippedItem(
    EquipmentSlotType slotType) const
{
    std::size_t slotIndex =
        static_cast<std::size_t>(slotType);

    if (slotIndex >= equippedItems.size())
    {
        return ItemType::NONE;
    }

    return equippedItems[slotIndex];
}

bool Equipment::IsSlotEmpty(
    EquipmentSlotType slotType) const
{
    return GetEquippedItem(slotType) ==
           ItemType::NONE;
}

bool Equipment::IsEquipped(
    ItemType itemType) const
{
    for (ItemType equippedItem :
         equippedItems)
    {
        if (equippedItem == itemType)
        {
            return true;
        }
    }

    return false;
}