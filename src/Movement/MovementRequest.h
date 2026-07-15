#pragma once


class MovementRequest
{
public:

    MovementRequest(int x, int y);


    int GetX();

    int GetY();


private:

    int x;

    int y;
};
