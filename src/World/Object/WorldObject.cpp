#include "WorldObject.h"

WorldObject::WorldObject(int id, int x, int y)
{
    this->id = id;
    this->x = x;
    this->y = y;
}

int WorldObject::GetID()
{
    return id;
}

int WorldObject::GetX()
{
    return x;
}

int WorldObject::GetY()
{
    return y;
}