#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class Map;
class Entity;
class ResourceNode;
class CraftingStation;

// Read-only landscape dressing. Its fields never consume simulation RNG or
// change collision, resource state, saved data, or the authored world layout.
class WorldArtVariation
{
public:
    struct Cell
    {
        int grass = 0, path = 0, water = 0, stone = 0;
        int plant = -1, leaves = -1, detail = -1;
        float wildness = 0, canopy = 0;
    };
    void Prepare(Map &map, const std::vector<ResourceNode> &resources,
                 const std::vector<CraftingStation> &stations,
                 const std::vector<std::unique_ptr<Entity>> &entities);
    const Cell &At(int x, int y) const;
    void Clear();
    static std::uint32_t Hash(int x, int y, std::uint32_t salt = 0);
    // 0=new, 1=weathered, 2=repaired. The workshop has an authored repair history.
    static int BuildingCondition(const char *id);
    static int BuildingMaterial(const char *id);

private:
    const Map *map = nullptr;
    std::uint64_t revision = 0;
    int width = 0, height = 0;
    std::size_t resourceCount = 0, stationCount = 0;
    std::vector<Cell> cells;
};
