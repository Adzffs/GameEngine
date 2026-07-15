#include "MovementRequest.h"


MovementRequest::MovementRequest(int x, int y)
{
    this->x = x;
    this->y = y;
}


int MovementRequest::GetX()
{
    return x;
}


int MovementRequest::GetY()
{
    return y;
}
