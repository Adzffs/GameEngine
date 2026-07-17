#include "ResourceDefinition.h"

#include <utility>

ResourceDefinition::ResourceDefinition(
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
    int respawnTicks)
    : resourceType(resourceType),
      name(std::move(name)),
      requiredSkill(requiredSkill),
      requiredSkillLevel(requiredSkillLevel),
      requiredToolType(requiredToolType),
      xpReward(xpReward),
      itemReward(itemReward),
      itemAmount(itemAmount),
      baseSuccessChance(baseSuccessChance),
      maxUses(maxUses),
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

ToolType ResourceDefinition::GetRequiredToolType() const
{
    return requiredToolType;
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

int ResourceDefinition::GetBaseSuccessChance() const
{
    return baseSuccessChance;
}

int ResourceDefinition::GetMaxUses() const
{
    return maxUses;
}

int ResourceDefinition::GetRespawnTicks() const
{
    return respawnTicks;
}