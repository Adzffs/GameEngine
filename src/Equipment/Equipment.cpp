#include "Equipment.h"

#include "../Item/ItemDatabase.h"

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

StatBlock Equipment::GetTotalStatBonuses() const
{
    StatBlock totalBonuses;

    for (ItemType equippedItem :
         equippedItems)
    {
        if (equippedItem == ItemType::NONE)
        {
            continue;
        }

        const ItemDefinition &itemDefinition =
            ItemDatabase::Get(equippedItem);

        totalBonuses +=
            itemDefinition.GetEquipmentStatBonuses();
    }

    return totalBonuses;
}