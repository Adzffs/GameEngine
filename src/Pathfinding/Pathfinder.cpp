#include "Pathfinder.h"
#include "../World/Map.h"

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <tuple>
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

    using SearchNode = std::tuple<int, int, int, int>;
    std::priority_queue<SearchNode, std::vector<SearchNode>,
                        std::greater<SearchNode>> frontier;
    std::set<Coordinate> visited;

    std::map<Coordinate, Coordinate> cameFrom;
    std::map<Coordinate, int> distanceFromStart;

    int insertionOrder = 0;
    const auto heuristic = [destinationX, destinationY](int x, int y)
    {
        return std::abs(x - destinationX) + std::abs(y - destinationY);
    };
    frontier.emplace(heuristic(startX, startY), insertionOrder++, startX, startY);
    distanceFromStart[start] = 0;

    Coordinate bestDestination = start;
    int bestDistance = heuristic(startX, startY);
    int bestPathLength = 0;

    const int directions[4][2]{
        {0, -1},
        {1, 0},
        {0, 1},
        {-1, 0}};

    bool exactPathFound = false;

    while (!frontier.empty())
    {
        const auto [priority, order, currentX, currentY] = frontier.top();
        frontier.pop();
        (void)priority;
        (void)order;
        const Coordinate current{currentX, currentY};
        if (visited.contains(current)) continue;
        visited.insert(current);

        const int distance = heuristic(current.first, current.second);

        int currentPathLength =
            distanceFromStart[current];

        if (distance < bestDistance ||
            (distance == bestDistance &&
             currentPathLength < bestPathLength))
        {
            bestDestination = current;
            bestDistance = distance;
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

            Coordinate next{
                nextX,
                nextY};

            if (visited.contains(next))
            {
                continue;
            }

            const int tentativeDistance = currentPathLength + 1;
            auto knownDistance = distanceFromStart.find(next);
            if (knownDistance != distanceFromStart.end() &&
                tentativeDistance >= knownDistance->second)
            {
                continue;
            }

            cameFrom[next] = current;
            distanceFromStart[next] = tentativeDistance;
            frontier.emplace(
                tentativeDistance + heuristic(nextX, nextY),
                insertionOrder++, nextX, nextY);
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
