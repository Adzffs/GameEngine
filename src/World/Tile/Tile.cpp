#include "Tile.h"

Tile::Tile()
{
    type = TileType::GRASS;

    blocked = false;
}

TileType Tile::GetType()
{
    return type;
}

TileType Tile::GetGroundType() const
{
    return groundType;
}

void Tile::SetType(TileType type)
{
    this->type = type;

    switch (type)
    {
    case TileType::GRASS:
    case TileType::DIRT:
    case TileType::STONE:
        groundType = type;
        blocked = false;
        break;

    case TileType::WATER:
        groundType = type;
        blocked = true;
        break;
    case TileType::TREE:
    case TileType::WALL:
    case TileType::STATION:
        blocked = true;
        break;
    }
}

bool Tile::IsBlocked()
{
    return blocked;
}
