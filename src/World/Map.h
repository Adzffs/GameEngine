#pragma once

#include "Tile/Tile.h"
#include "Building.h"
#include <cstdint>
#include <vector>

class Map
{
public:
    Map(int width, int height);

    bool IsValidPosition(int x, int y);
    bool IsInBounds(int x, int y) const;
    int GetWidth() const;
    int GetHeight() const;

    void SetTileType(int x, int y, TileType type);
    TileType GetTileType(int x, int y);
    // Objects retain their collision type while preserving the authored floor.
    TileType GetGroundType(int x, int y);
    void SetBuildings(std::vector<Building> layout);
    const std::vector<Building> &GetBuildings() const { return buildings; }
    const Building *GetBuildingAt(int x, int y) const;
    std::uint64_t GetRevision() const { return revision; }

private:
    int width;

    int height;

    std::vector<Tile> tiles;
    std::vector<Building> buildings;
    std::uint64_t revision = 0;

    Tile &GetTile(int x, int y);
};
