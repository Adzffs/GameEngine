#include "ResourceDefinition.h"

#include <utility>

ResourceDefinition::ResourceDefinition(
    ResourceType resourceType,
    std::string name,
    SkillType requiredSkill,
    int requiredSkillLevel,
    int xpReward,
    ItemType itemReward,
    int itemAmount,
    int respawnTicks)
    : resourceType(resourceType),
      name(std::move(name)),
      requiredSkill(requiredSkill),
      requiredSkillLevel(requiredSkillLevel),
      xpReward(xpReward),
      itemReward(itemReward),
      itemAmount(itemAmount),
      respawnTicks(respawnTicks)
{
}

ResourceType
ResourceDefinition::GetResourceType() const
{
    return resourceType;
}

const std::string &
ResourceDefinition::GetName() const
{
    return name;
}

SkillType
ResourceDefinition::GetRequiredSkill() const
{
    return requiredSkill;
}

int ResourceDefinition::GetRequiredSkillLevel() const
{
    return requiredSkillLevel;
}

int ResourceDefinition::GetXPReward() const
{
    return xpReward;
}

ItemType ResourceDefinition::GetItemReward() const
{
    return itemReward;
}

int ResourceDefinition::GetItemAmount() const
{
    return itemAmount;
}

int ResourceDefinition::GetRespawnTicks() const
{
    return respawnTicks;
}