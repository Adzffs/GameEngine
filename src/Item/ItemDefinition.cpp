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
    int actionDurationTicks,
    StatBlock equipmentStatBonuses)
    : itemType(itemType),
      name(std::move(name)),
      stackable(stackable),
      equipmentSlot(equipmentSlot),
      toolType(toolType),
      requiredSkill(requiredSkill),
      requiredSkillLevel(requiredSkillLevel),
      actionDurationTicks(actionDurationTicks),
      equipmentStatBonuses(equipmentStatBonuses)
{
    if (requiredSkill != SkillType::NONE ||
        requiredSkillLevel != 0)
    {
        requirements.emplace_back(
            RequirementSystem::SkillLevelRequirement{
                requiredSkill,
                requiredSkillLevel});
    }
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

const std::vector<RequirementSystem::Requirement> &
ItemDefinition::GetRequirements() const
{
    return requirements;
}

const StatBlock &ItemDefinition::GetEquipmentStatBonuses() const
{
    return equipmentStatBonuses;
}