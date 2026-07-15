#pragma once

#include "Tile/Tile.h"

class Map
{
public:
    Map(int width, int height);

    bool IsValidPosition(int x, int y);

private:
    int width;

    int height;

    Tile **tiles;
};