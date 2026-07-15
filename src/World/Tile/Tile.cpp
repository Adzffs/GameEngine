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

void Tile::SetType(TileType type)
{
    this->type = type;

    switch (type)
    {
    case TileType::GRASS:
    case TileType::DIRT:
    case TileType::STONE:
        blocked = false;
        break;

    case TileType::WATER:
    case TileType::TREE:
    case TileType::WALL:
        blocked = true;
        break;
    }
}

bool Tile::IsBlocked()
{
    return blocked;
}