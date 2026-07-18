#pragma once

class Position
{
public:
    Position(int x, int y);

    int GetX();
    int GetX() const;

    int GetY();
    int GetY() const;

    void SetPosition(int x, int y);

private:
    int x;
    int y;
};