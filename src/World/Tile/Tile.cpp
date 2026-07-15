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
}

bool Tile::IsBlocked()
{
    return blocked;
}