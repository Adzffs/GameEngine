#pragma once

class WorldObject
{
public:
    WorldObject(int id, int x, int y);

    int GetID();

    int GetX();

    int GetY();

private:
    int id;

    int x;

    int y;
};