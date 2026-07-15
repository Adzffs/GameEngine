#include "Tile.h"

Tile::Tile()
{
    blocked = false;
}

bool Tile::IsBlocked()
{
    return blocked;
}

void Tile::SetBlocked(bool blocked)
{
    this->blocked = blocked;
}