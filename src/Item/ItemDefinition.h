#pragma once

#include "../Inventory/ItemType.h"
#include "../Equipment/EquipmentSlotType.h"
#include "../Skills/SkillType.h"
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
        SkillType requiredSkill,
        int requiredSkillLevel,
        int actionDurationTicks);

    ItemType GetItemType() const;
    const std::string &GetName() const;

    bool IsStackable() const;
    bool IsEquippable() const;
    bool HasSkillRequirement() const;

    EquipmentSlotType GetEquipmentSlot() const;
    ToolType GetToolType() const;

    SkillType GetRequiredSkill() const;
    int GetRequiredSkillLevel() const;

    int GetActionDurationTicks() const;

private:
    ItemType itemType;
    std::string name;
    bool stackable;

    EquipmentSlotType equipmentSlot;
    ToolType toolType;

    SkillType requiredSkill;
    int requiredSkillLevel;

    int actionDurationTicks;
};