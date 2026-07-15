#pragma once

class Position
{
public:
    Position(int x, int y);

    int GetX();

    int GetY();

    void SetPosition(int x, int y);

private:
    int x;
    int y;
};