#pragma once

class Tile
{
public:
    Tile();

    bool IsBlocked();

    void SetBlocked(bool blocked);

private:
    bool blocked;
};