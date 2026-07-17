#pragma once

#include "ResourceType.h"
#include "../../../Inventory/ItemType.h"
#include "../../../Skills/SkillType.h"

#include <string>

class ResourceDefinition
{
public:
    ResourceDefinition(
        ResourceType resourceType,
        std::string name,
        SkillType requiredSkill,
        int requiredSkillLevel,
        int xpReward,
        ItemType itemReward,
        int itemAmount,
        int respawnTicks);

    ResourceType GetResourceType() const;
    const std::string &GetName() const;

    SkillType GetRequiredSkill() const;
    int GetRequiredSkillLevel() const;

    int GetXPReward() const;

    ItemType GetItemReward() const;
    int GetItemAmount() const;

    int GetRespawnTicks() const;

private:
    ResourceType resourceType;
    std::string name;

    SkillType requiredSkill;
    int requiredSkillLevel;

    int xpReward;

    ItemType itemReward;
    int itemAmount;

    int respawnTicks;
};