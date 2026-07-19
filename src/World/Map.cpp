#include "Map.h"

Map::Map(int width, int height)
{
    this->width = width;
    this->height = height;

    tiles.resize(static_cast<size_t>(width) * height);

    if (width > 10 && height > 10)
    {
        GetTile(10, 10).SetType(TileType::WATER);
    }
}
void Map::SetTileType(int x, int y, TileType type)
{
    GetTile(x, y).SetType(type);
}

Tile &Map::GetTile(int x, int y)
{
    return tiles[static_cast<size_t>(y) * width + x];
}

bool Map::IsValidPosition(int x, int y)
{
    if (!IsInBounds(x, y))
        return false;

    return !GetTile(x, y).IsBlocked();
}

bool Map::IsInBounds(int x, int y) const
{
    return x >= 0 && y >= 0 &&
           x < width && y < height;
}
