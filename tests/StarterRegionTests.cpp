#include "TestSupport.h"

#include "../src/World/Development/DevelopmentWorldContent.h"
#include "../src/World/World.h"

#include <array>
#include <queue>
#include <set>
#include <utility>

namespace
{
    bool CanReachAdjacentTile(Map &map, int targetX, int targetY)
    {
        using Point = std::pair<int, int>;
        std::queue<Point> frontier;
        std::set<Point> visited;
        const Point start{DevelopmentWorldContent::PlayerSpawnX,
                          DevelopmentWorldContent::PlayerSpawnY};
        frontier.push(start);
        visited.insert(start);
        const std::array<Point, 4> directions{{
            {1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

        while (!frontier.empty())
        {
            const auto [x, y] = frontier.front();
            frontier.pop();
            if (std::abs(x - targetX) + std::abs(y - targetY) == 1)
            {
                return true;
            }
            for (const auto [offsetX, offsetY] : directions)
            {
                const Point next{x + offsetX, y + offsetY};
                if (map.IsValidPosition(next.first, next.second) &&
                    !visited.contains(next))
                {
                    visited.insert(next);
                    frontier.push(next);
                }
            }
        }
        return false;
    }
}

int main()
{
    TestContext test;
    World world;
    Map &map = world.GetMap();

    test.Expect(map.GetTileType(2, 2) == TileType::STONE,
        "Village square has its own stone material identity");
    test.Expect(map.GetTileType(4, 6) == TileType::DIRT,
        "Woodland route is marked by a dirt path");
    test.Expect(map.GetTileType(6, 10) == TileType::STONE,
        "Mine uses a distinct stone floor");
    test.Expect(map.GetTileType(15, 10) == TileType::STONE,
        "Workshop uses a distinct stone floor");
    test.Expect(map.GetTileType(10, 10) == TileType::WATER,
        "Existing pond remains a recognizable blocked landmark");

    for (const auto &resource :
         DevelopmentWorldContent::GetStarterResourcePlacements())
    {
        test.Expect(CanReachAdjacentTile(map, resource.x, resource.y),
            "Every starter resource has a reachable interaction tile");
    }
    for (const auto &station :
         DevelopmentWorldContent::GetStarterStationPlacements())
    {
        test.Expect(CanReachAdjacentTile(map, station.x, station.y),
            "Every starter service has a reachable interaction tile");
    }

    return test.Finish();
}
