#include "ResourceNode.h"

#include <iostream>

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

        std::cout
            << "Resource "
            << GetID()
            << " respawning in "
            << remainingRespawnTicks
            << " ticks."
            << std::endl;
    }

    if (remainingRespawnTicks == 0)
    {
        active = true;

        std::cout
            << "Resource "
            << GetID()
            << " has respawned."
            << std::endl;
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

    std::cout
        << "Resource "
        << GetID()
        << " has been depleted."
        << std::endl;
}

bool ResourceNode::IsActive() const
{
    return active;
}

int ResourceNode::GetRemainingRespawnTicks() const
{
    return remainingRespawnTicks;
}