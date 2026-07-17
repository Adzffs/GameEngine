#pragma once

#include "../WorldObject.h"
#include "StationType.h"

class CraftingStation : public WorldObject
{
public:
    CraftingStation(
        int id,
        StationType stationType,
        int x,
        int y);

    StationType GetStationType() const;

private:
    StationType stationType;
};
