#pragma once

class WorldObject
{
public:
    WorldObject(int id, int x, int y);

    int GetID() const;
    int GetX() const;
    int GetY() const;

private:
    int id;

    int x;

    int y;
};