#include "Position.h"

Position::Position(int x, int y)
{
    this->x = x;
    this->y = y;
}

int Position::GetX()
{
    return x;
}

int Position::GetY()
{
    return y;
}

void Position::SetPosition(int x, int y)
{
    this->x = x;
    this->y = y;
}