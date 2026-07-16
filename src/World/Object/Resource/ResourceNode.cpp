#include "ResourceNode.h"

ResourceNode::ResourceNode(
    int id,
    int x,
    int y,
    int respawnTicks)
    : WorldObject(id, x, y),
      active(true),
      respawnTicks(respawnTicks),
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
    remainingRespawnTicks = respawnTicks;
}

bool ResourceNode::IsActive() const
{
    return active;
}

int ResourceNode::GetRemainingRespawnTicks() const
{
    return remainingRespawnTicks;
}