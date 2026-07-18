#pragma once

#include "ResourceType.h"
#include "DepletedVisualType.h"
#include "../../../Inventory/ItemType.h"
#include "../../../Skills/SkillType.h"
#include "../../../Item/ToolType.h"
#include "../../../Requirement/Requirement.h"

#include <vector>
#include <string>

class ResourceDefinition
{
public:
    ResourceDefinition(
        ResourceType resourceType,
        std::string name,
        SkillType requiredSkill,
        int requiredSkillLevel,
        ToolType requiredToolType,
        int xpReward,
        ItemType itemReward,
        int itemAmount,
        int baseSuccessChance,
        int maxUses,
        DepletedVisualType depletedVisualType,
        int respawnTicks);

    ResourceType GetResourceType() const;
    const std::string &GetName() const;

    SkillType GetRequiredSkill() const;
    int GetRequiredSkillLevel() const;
    ToolType GetRequiredToolType() const;
    int GetXPReward() const;

    ItemType GetItemReward() const;
    int GetItemAmount() const;

    int GetBaseSuccessChance() const;

    int GetMaxUses() const;
    DepletedVisualType GetDepletedVisualType() const;

    int GetRespawnTicks() const;

    const std::vector<RequirementSystem::Requirement> &
    GetRequirements() const;

private:
    ResourceType resourceType;
    std::string name;

    SkillType requiredSkill;
    int requiredSkillLevel;
    ToolType requiredToolType;

    int xpReward;

    ItemType itemReward;
    int itemAmount;

    int baseSuccessChance;

    int maxUses;
    DepletedVisualType depletedVisualType;

    int respawnTicks;

    std::vector<RequirementSystem::Requirement> requirements;
};
