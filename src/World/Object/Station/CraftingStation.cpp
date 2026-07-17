#include "CraftingStation.h"

CraftingStation::CraftingStation(
    int id,
    StationType stationType,
    int x,
    int y)
    : WorldObject(id, x, y),
      stationType(stationType)
{
}

StationType
CraftingStation::GetStationType() const
{
    return stationType;
}
