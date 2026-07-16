#include "WorldObject.h"

WorldObject::WorldObject(int id, int x, int y)
{
    this->id = id;
    this->x = x;
    this->y = y;
}

int WorldObject::GetID() const
{
    return id;
}

int WorldObject::GetX() const
{
    return x;
}

int WorldObject::GetY() const
{
    return y;
}