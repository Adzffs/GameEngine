#pragma once

#include "../WorldObject.h"

class ResourceNode : public WorldObject
{
public:
    ResourceNode(int id, int x, int y);

    void Gather();

private:
    int amount;
};