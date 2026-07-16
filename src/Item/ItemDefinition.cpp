#include "ItemDefinition.h"

#include <utility>

ItemDefinition::ItemDefinition(
    ItemType itemType,
    std::string name,
    bool stackable,
    EquipmentSlotType equipmentSlot,
    ToolType toolType,
    int actionDurationTicks)
    : itemType(itemType),
      name(std::move(name)),
      stackable(stackable),
      equipmentSlot(equipmentSlot),
      toolType(toolType),
      actionDurationTicks(actionDurationTicks)
{
}

ItemType ItemDefinition::GetItemType() const
{
    return itemType;
}

const std::string &ItemDefinition::GetName() const
{
    return name;
}

bool ItemDefinition::IsStackable() const
{
    return stackable;
}

bool ItemDefinition::IsEquippable() const
{
    return equipmentSlot !=
           EquipmentSlotType::NONE;
}

EquipmentSlotType
ItemDefinition::GetEquipmentSlot() const
{
    return equipmentSlot;
}

ToolType ItemDefinition::GetToolType() const
{
    return toolType;
}

int ItemDefinition::GetActionDurationTicks() const
{
    return actionDurationTicks;
}