#pragma once

#include <vector>

class Map;

struct PathStep
{
    int x;
    int y;
};

class Pathfinder
{
public:
    std::vector<PathStep> FindPath(
        Map &map,
        int startX,
        int startY,
        int destinationX,
        int destinationY);
};