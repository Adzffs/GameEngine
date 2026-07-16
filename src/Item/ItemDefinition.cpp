#include "ItemDefinition.h"

#include <utility>

ItemDefinition::ItemDefinition(
    ItemType itemType,
    std::string name,
    bool stackable,
    EquipmentSlotType equipmentSlot,
    ToolType toolType,
    SkillType requiredSkill,
    int requiredSkillLevel,
    int actionDurationTicks)
    : itemType(itemType),
      name(std::move(name)),
      stackable(stackable),
      equipmentSlot(equipmentSlot),
      toolType(toolType),
      requiredSkill(requiredSkill),
      requiredSkillLevel(requiredSkillLevel),
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

bool ItemDefinition::HasSkillRequirement() const
{
    return requiredSkill != SkillType::NONE &&
           requiredSkillLevel > 0;
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

SkillType ItemDefinition::GetRequiredSkill() const
{
    return requiredSkill;
}

int ItemDefinition::GetRequiredSkillLevel() const
{
    return requiredSkillLevel;
}

int ItemDefinition::GetActionDurationTicks() const
{
    return actionDurationTicks;
}