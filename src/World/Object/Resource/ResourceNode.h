#pragma once

#include "../WorldObject.h"

class ResourceNode : public WorldObject
{
public:
    ResourceNode(
        int id,
        int x,
        int y,
        int respawnTicks = 10);

    void Update();

    void Deplete();

    bool IsActive() const;
    int GetRemainingRespawnTicks() const;

private:
    bool active;

    int respawnTicks;
    int remainingRespawnTicks;
};