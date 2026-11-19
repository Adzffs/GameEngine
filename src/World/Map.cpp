#include "Map.h"
#include <stdexcept>
#include <utility>

const Building *Map::GetBuildingAt(int x, int y) const
{
    for (const auto &b : buildings) if (b.Contains(x, y)) return &b;
    return nullptr;
}

void Map::SetBuildings(std::vector<Building> layout)
{
    for (const auto &b : layout)
        if (b.x < 0 || b.y < 0 || b.width < 5 || b.width > 16 || b.height < 4 || b.height > 12 ||
            b.x > width - b.width || b.y > height - b.height || b.doorOffset < 1 || b.doorOffset >= b.width - 1)
            throw std::invalid_argument("Invalid building map bounds");
    for (const auto &b : buildings)
        for (int y = b.y; y < b.y + b.height; ++y)
            for (int x = b.x; x < b.x + b.width; ++x)
                if (b.IsWall(x, y) && GetTile(x, y).GetType() == TileType::WALL)
                    GetTile(x, y).SetType(GetTile(x, y).GetGroundType());
    buildings = std::move(layout);
    ++revision;
    for (const auto &b : buildings)
        for (int y = b.y; y < b.y + b.height; ++y)
            for (int x = b.x; x < b.x + b.width; ++x)
                if (b.IsWall(x, y)) GetTile(x, y).SetType(TileType::WALL);
}

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
    ++revision;
}

TileType Map::GetTileType(int x, int y)
{
    return GetTile(x, y).GetType();
}

Tile &Map::GetTile(int x, int y)
{
    return tiles[static_cast<size_t>(y) * width + x];
}

TileType Map::GetGroundType(int x, int y)
{
    return GetTile(x, y).GetGroundType();
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

int Map::GetWidth() const
{
    return width;
}

int Map::GetHeight() const
{
    return height;
}
