#include "Movement.h"
#include "../World/Map.h"

bool Movement::Move(Entity &entity, Map &map, int x, int y)
{
    Position &position = entity.GetPosition();

    int newX = position.GetX() + x;
    int newY = position.GetY() + y;

    if (!map.IsValidPosition(newX, newY))
    {
        return false;
    }

    position.SetPosition(newX, newY);

    return true;
}