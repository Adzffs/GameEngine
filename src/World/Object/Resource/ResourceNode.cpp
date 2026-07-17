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
    }
}

void ResourceNode::Deplete()
{
    if (!active)
    {
        return;
    }

    active = false;

    remainingRespawnTicks =
        respawnTicks;
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