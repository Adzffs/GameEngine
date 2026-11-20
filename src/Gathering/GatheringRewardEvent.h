#pragma once

#include "../Inventory/ItemType.h"
#include "../World/Object/Resource/ResourceType.h"

// Emitted only after the world commits a gathered item, XP and resource use.
// Published for one tick; clients consume every simulation update, including catch-up.
struct GatheringRewardEvent
{
    int actorEntityID;
    int resourceID;
    int completionTick;
    ResourceType resourceType;
    int resourceX, resourceY;
    int actorX, actorY;
    ItemType itemType;
    int amount;
    int xp;
};
