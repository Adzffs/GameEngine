#include "Pathfinder.h"
#include "../World/Map.h"

#include <algorithm>
#include <limits>
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

    const Coordinate start{
        startX,
        startY};

    const Coordinate destination{
        destinationX,
        destinationY};

    std::queue<Coordinate> frontier;
    std::set<Coordinate> visited;

    std::map<Coordinate, Coordinate> cameFrom;
    std::map<Coordinate, int> distanceFromStart;

    frontier.push(start);
    visited.insert(start);
    distanceFromStart[start] = 0;

    Coordinate bestDestination = start;

    int bestDistanceSquared =
        std::numeric_limits<int>::max();

    int bestPathLength =
        std::numeric_limits<int>::max();

    const int directions[8][2]{
        {0, -1},
        {1, -1},
        {1, 0},
        {1, 1},
        {0, 1},
        {-1, 1},
        {-1, 0},
        {-1, -1}};

    bool exactPathFound = false;

    while (!frontier.empty())
    {
        Coordinate current = frontier.front();
        frontier.pop();

        int differenceX =
            current.first - destinationX;

        int differenceY =
            current.second - destinationY;

        int distanceSquared =
            differenceX * differenceX +
            differenceY * differenceY;

        int currentPathLength =
            distanceFromStart[current];

        if (distanceSquared < bestDistanceSquared ||
            (distanceSquared == bestDistanceSquared &&
             currentPathLength < bestPathLength))
        {
            bestDestination = current;
            bestDistanceSquared = distanceSquared;
            bestPathLength = currentPathLength;
        }

        if (current == destination)
        {
            exactPathFound = true;
            bestDestination = destination;
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

            if (changeX != 0 && changeY != 0)
            {
                bool horizontalTileIsValid =
                    map.IsValidPosition(
                        current.first + changeX,
                        current.second);

                bool verticalTileIsValid =
                    map.IsValidPosition(
                        current.first,
                        current.second + changeY);

                if (!horizontalTileIsValid ||
                    !verticalTileIsValid)
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

            distanceFromStart[next] =
                currentPathLength + 1;

            frontier.push(next);
        }
    }

    Coordinate routeDestination;

    if (exactPathFound)
    {
        routeDestination = destination;
    }
    else
    {
        routeDestination = bestDestination;
    }

    if (routeDestination == start)
    {
        return path;
    }

    Coordinate current = routeDestination;

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