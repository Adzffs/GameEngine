#pragma once

#include "../Inventory/ItemType.h"
#include "../Equipment/EquipmentSlotType.h"
#include "ToolType.h"

#include <string>

class ItemDefinition
{
public:
    ItemDefinition(
        ItemType itemType,
        std::string name,
        bool stackable,
        EquipmentSlotType equipmentSlot,
        ToolType toolType,
        int actionDurationTicks);

    ItemType GetItemType() const;
    const std::string &GetName() const;

    bool IsStackable() const;
    bool IsEquippable() const;

    EquipmentSlotType GetEquipmentSlot() const;
    ToolType GetToolType() const;

    int GetActionDurationTicks() const;

private:
    ItemType itemType;
    std::string name;
    bool stackable;

    EquipmentSlotType equipmentSlot;
    ToolType toolType;

    int actionDurationTicks;
};