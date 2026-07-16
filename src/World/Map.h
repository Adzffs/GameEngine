#pragma once

#include "Tile/Tile.h"
#include <vector>

class Map
{
public:
    Map(int width, int height);

    bool IsValidPosition(int x, int y);

private:
    int width;

    int height;

    std::vector<Tile> tiles;

    Tile &GetTile(int x, int y);
};