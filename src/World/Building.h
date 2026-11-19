#pragma once

#include <string>

// Authored rectangular timber/stone shell. A permanently open south doorway
// is the only perimeter gap; roof visibility never changes collision.
struct Building
{
    std::string id;
    int x = 0, y = 0, width = 0, height = 0, doorOffset = 0;

    bool Contains(int tx, int ty) const
    {
        return tx >= x && ty >= y && tx < x + width && ty < y + height;
    }
    bool IsDoor(int tx, int ty) const
    {
        return tx == x + doorOffset && ty == y + height - 1;
    }
    bool IsWall(int tx, int ty) const
    {
        return Contains(tx, ty) && !IsDoor(tx, ty) &&
            (tx == x || ty == y || tx == x + width - 1 || ty == y + height - 1);
    }
};
