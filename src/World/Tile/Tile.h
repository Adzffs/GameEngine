#pragma once

#include "TileType.h"

class Tile
{
public:
    Tile();

    TileType GetType();
    TileType GetGroundType() const;

    void SetType(TileType type);

    bool IsBlocked();

private:
    TileType type;
    TileType groundType = TileType::GRASS;

    bool blocked;
};
