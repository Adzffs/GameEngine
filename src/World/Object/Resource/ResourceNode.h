#pragma once

#include "../WorldObject.h"
#include "ResourceType.h"

class ResourceNode : public WorldObject
{
public:
    ResourceNode(
        int id,
        ResourceType resourceType,
        int x,
        int y);

    void Update();
    void Deplete();

    bool IsActive() const;

    ResourceType GetResourceType() const;

    int GetRemainingRespawnTicks() const;

private:
    ResourceType resourceType;

    bool active;

    int respawnTicks;
    int remainingRespawnTicks;
};