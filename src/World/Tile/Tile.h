#pragma once

#include "TileType.h"

class Tile
{
public:
    Tile();

    TileType GetType();

    void SetType(TileType type);

    bool IsBlocked();

private:
    TileType type;

    bool blocked;
};