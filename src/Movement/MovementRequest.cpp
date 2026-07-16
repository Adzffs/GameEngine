#include "MovementRequest.h"

MovementRequest::MovementRequest(int entityID, int x, int y)
    : entityID(entityID),
      x(x),
      y(y)
{
}

int MovementRequest::GetEntityID() const
{
    return entityID;
}

int MovementRequest::GetX() const
{
    return x;
}

int MovementRequest::GetY() const
{
    return y;
}