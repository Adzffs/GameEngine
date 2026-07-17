#include "ResourceNode.h"
#include "ResourceDatabase.h"

ResourceNode::ResourceNode(
    int id,
    ResourceType resourceType,
    int x,
    int y)
    : WorldObject(id, x, y),
      resourceType(resourceType),
      active(true),
      maxUses(
          ResourceDatabase::Get(resourceType)
              .GetMaxUses()),
      remainingUses(maxUses),
      respawnTicks(
          ResourceDatabase::Get(resourceType)
              .GetRespawnTicks()),
      remainingRespawnTicks(0)
{
}

void ResourceNode::Update()
{
    if (active)
    {
        return;
    }

    if (remainingRespawnTicks > 0)
    {
        remainingRespawnTicks--;
    }

    if (remainingRespawnTicks == 0)
    {
        active = true;
        remainingUses = maxUses;
    }
}

void ResourceNode::ConsumeUse()
{
    if (!active)
    {
        return;
    }

    if (remainingUses > 0)
    {
        remainingUses--;
    }

    if (remainingUses > 0)
    {
        return;
    }

    active = false;
    remainingRespawnTicks = respawnTicks;
}

bool ResourceNode::IsActive() const
{
    return active;
}

ResourceType ResourceNode::GetResourceType() const
{
    return resourceType;
}

int ResourceNode::GetRemainingRespawnTicks() const
{
    return remainingRespawnTicks;
}
int ResourceNode::GetMaxUses() const
{
    return maxUses;
}

int ResourceNode::GetRemainingUses() const
{
    return remainingUses;
}