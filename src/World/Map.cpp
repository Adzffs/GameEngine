#include "Map.h"

Map::Map(int width, int height)
{
    this->width = width;
    this->height = height;

    tiles = new Tile *[width];

    for (int x = 0; x < width; x++)
    {
        tiles[x] = new Tile[height];
        }
    tiles[5][5].SetType(TileType::TREE);
    tiles[10][10].SetType(TileType::WATER);
}

bool Map::IsValidPosition(int x, int y)
{
    if (x < 0 || y < 0)
        return false;

    if (x >= width || y >= height)
        return false;

    return !tiles[x][y].IsBlocked();
}
