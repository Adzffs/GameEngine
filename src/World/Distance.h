#pragma once

#include <cmath>

class Distance
{
public:
    static int Calculate(
        int x1,
        int y1,
        int x2,
        int y2)
    {
        return abs(x1 - x2) + abs(y1 - y2);
    }
};