#include "Movement.h"
#include "../World/Map.h"

void Movement::Move(Entity &entity, int x, int y)
{
    Position &position = entity.GetPosition();

    position.SetPosition(
        position.GetX() + x,
        position.GetY() + y);
}