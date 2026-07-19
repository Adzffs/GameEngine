#pragma once

#include "Tile/Tile.h"
#include <vector>

class Map
{
public:
    Map(int width, int height);

    bool IsValidPosition(int x, int y);
    bool IsInBounds(int x, int y) const;

    void SetTileType(int x, int y, TileType type);

private:
    int width;

    int height;

    std::vector<Tile> tiles;

    Tile &GetTile(int x, int y);
};
