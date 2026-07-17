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
    void ConsumeUse();

    bool IsActive() const;

    ResourceType GetResourceType() const;

    int GetMaxUses() const;
    int GetRemainingUses() const;
    int GetRemainingRespawnTicks() const;

private:
    ResourceType resourceType;

    bool active;

    int maxUses;
    int remainingUses;

    int respawnTicks;
    int remainingRespawnTicks;
};