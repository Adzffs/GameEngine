#include "Pathfinder.h"
#include "../World/Map.h"

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <utility>

std::vector<PathStep> Pathfinder::FindPath(
    Map &map,
    int startX,
    int startY,
    int destinationX,
    int destinationY)
{
    using Coordinate = std::pair<int, int>;

    std::vector<PathStep> path;

    if (startX == destinationX &&
        startY == destinationY)
    {
        return path;
    }

    if (!map.IsValidPosition(
            destinationX,
            destinationY))
    {
        return path;
    }

    const Coordinate start{
        startX,
        startY};

    const Coordinate destination{
        destinationX,
        destinationY};

    std::queue<Coordinate> frontier;
    std::set<Coordinate> visited;
    std::map<Coordinate, Coordinate> cameFrom;

    frontier.push(start);
    visited.insert(start);

    const int directions[8][2]{
        {0, -1},
        {1, -1},
        {1, 0},
        {1, 1},
        {0, 1},
        {-1, 1},
        {-1, 0},
        {-1, -1}};

    bool pathFound = false;

    while (!frontier.empty())
    {
        Coordinate current = frontier.front();
        frontier.pop();

        if (current == destination)
        {
            pathFound = true;
            break;
        }

        for (const auto &direction : directions)
        {
            int changeX = direction[0];
            int changeY = direction[1];

            int nextX =
                current.first + changeX;

            int nextY =
                current.second + changeY;

            if (!map.IsValidPosition(nextX, nextY))
            {
                continue;
            }

            // Prevent diagonal movement through blocked corners.
            if (changeX != 0 && changeY != 0)
            {
                if (!map.IsValidPosition(
                        current.first + changeX,
                        current.second) ||
                    !map.IsValidPosition(
                        current.first,
                        current.second + changeY))
                {
                    continue;
                }
            }

            Coordinate next{
                nextX,
                nextY};

            if (visited.contains(next))
            {
                continue;
            }

            visited.insert(next);
            cameFrom[next] = current;
            frontier.push(next);
        }
    }

    if (!pathFound)
    {
        return path;
    }

    Coordinate current = destination;

    while (current != start)
    {
        path.push_back(
            PathStep{
                current.first,
                current.second});

        current = cameFrom.at(current);
    }

    std::reverse(
        path.begin(),
        path.end());

    return path;
}