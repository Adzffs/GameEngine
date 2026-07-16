#pragma once

class MovementRequest
{
public:
    MovementRequest(int entityID, int x, int y);

    int GetEntityID() const;
    int GetX() const;
    int GetY() const;

private:
    int entityID;
    int x;
    int y;
};