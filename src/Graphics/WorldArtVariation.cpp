#include "WorldArtVariation.h"
#include "../World/Map.h"
#include "../World/Object/Resource/ResourceNode.h"
#include "../World/Object/Station/CraftingStation.h"
#include "../NPC/NPC.h"
#include "../NPC/NpcDefinitionDatabase.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <string_view>

namespace
{
    float Smooth(float t) { return t * t * (3.0f - 2.0f * t); }
    float Patch(float x, float y, int scale, unsigned salt)
    {
        const int ix = static_cast<int>(std::floor(x / scale));
        const int iy = static_cast<int>(std::floor(y / scale));
        const float fx = Smooth(x / scale - ix), fy = Smooth(y / scale - iy);
        const auto value = [salt](int a, int b) { return (WorldArtVariation::Hash(a, b, salt) % 65536) / 65535.0f; };
        const float top = std::lerp(value(ix, iy), value(ix + 1, iy), fx);
        const float bottom = std::lerp(value(ix, iy + 1), value(ix + 1, iy + 1), fx);
        return std::lerp(top, bottom, fy);
    }
}

std::uint32_t WorldArtVariation::Hash(int x, int y, std::uint32_t salt)
{
    std::uint32_t h = static_cast<std::uint32_t>(x) * 73856093U ^
        static_cast<std::uint32_t>(y) * 19349663U ^ salt * 83492791U;
    h ^= h >> 16; h *= 0x7feb352dU; h ^= h >> 15; h *= 0x846ca68bU;
    return h ^ (h >> 16);
}

int WorldArtVariation::BuildingCondition(const char *id)
{
    if (std::string_view(id) == "building.workshop.001") return 2;
    std::uint32_t h = 2166136261U;
    for (; *id; ++id) h = (h ^ static_cast<unsigned char>(*id)) * 16777619U;
    return static_cast<int>(h % 3);
}

int WorldArtVariation::BuildingMaterial(const char *id)
{
    // Preserve the workshop's terracotta identity; other stable IDs can use slate.
    if (std::string_view(id) == "building.workshop.001") return 0;
    std::uint32_t h = 0;
    for (; *id; ++id) h = h * 33U + static_cast<unsigned char>(*id);
    return static_cast<int>(h % 2);
}

void WorldArtVariation::Clear() { map = nullptr; cells.clear(); width = height = 0; }

const WorldArtVariation::Cell &WorldArtVariation::At(int x, int y) const
{
    static const Cell empty{};
    return x >= 0 && y >= 0 && x < width && y < height ? cells[y * width + x] : empty;
}

void WorldArtVariation::Prepare(Map &world, const std::vector<ResourceNode> &resources,
    const std::vector<CraftingStation> &stations, const std::vector<std::unique_ptr<Entity>> &entities)
{
    if (map == &world && revision == world.GetRevision() && resourceCount == resources.size() &&
        stationCount == stations.size()) return;
    map = &world; revision = world.GetRevision(); width = world.GetWidth(); height = world.GetHeight();
    resourceCount = resources.size(); stationCount = stations.size();
    const int size = width * height;
    cells.assign(size, {});
    std::vector<int> wet(size, 10000), depth(size, 10000), road(size, 10000), settlement(size, 10000), ore(size, 10000);
    std::vector<bool> occupied(size, false);
    const auto seed = [&](std::vector<int> &field, int x, int y)
    { if (world.IsInBounds(x, y)) field[y * width + x] = 0; };
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x)
    {
        const auto ground = world.GetGroundType(x, y);
        if (ground == TileType::WATER) seed(wet, x, y);
        else seed(depth, x, y);
        if (ground == TileType::DIRT || ground == TileType::STONE) seed(road, x, y);
        if (world.GetBuildingAt(x, y)) { seed(settlement, x, y); occupied[y * width + x] = true; }
    }
    for (const auto &station : stations)
    { seed(settlement, station.GetX(), station.GetY()); occupied[station.GetY() * width + station.GetX()] = true; }
    for (const auto &entity : entities)
        if (const auto *npc = dynamic_cast<const NPC *>(entity.get()))
            if (const auto *definition = NpcDefinitionDatabase::TryGet(npc->GetNpcType());
                definition && definition->kind == NpcKind::FRIENDLY)
            {
                const int x = npc->GetPosition().GetX(), y = npc->GetPosition().GetY();
                seed(settlement, x, y);
                if (world.IsInBounds(x, y)) occupied[y * width + x] = true;
            }
    for (const auto &node : resources)
    {
        const int tx = node.GetX(), ty = node.GetY();
        if (!world.IsInBounds(tx, ty)) continue;
        occupied[ty * width + tx] = true;
        const auto type = node.GetResourceType();
        const bool tree = type == ResourceType::NORMAL_TREE || type == ResourceType::OAK_TREE || type == ResourceType::WILLOW_TREE;
        if (!tree) { seed(ore, tx, ty); continue; }
        // Oak/willow canopies cover more ground. Litter persists at the stump.
        const float radius = type == ResourceType::NORMAL_TREE ? 2.15f : 2.75f;
        for (int y = std::max(0, ty - 3); y <= std::min(height - 1, ty + 3); ++y)
            for (int x = std::max(0, tx - 3); x <= std::min(width - 1, tx + 3); ++x)
            {
                const float dx = (x - tx) / radius, dy = (y - ty + 0.35f) / (radius * 0.8f);
                const float cover = std::max(0.0f, 1.0f - std::sqrt(dx * dx + dy * dy));
                cells[y * width + x].canopy = std::max(cells[y * width + x].canopy, cover);
            }
    }
    const auto spread = [&](std::vector<int> &field)
    {
        std::queue<int> pending;
        for (int i = 0; i < size; ++i) if (field[i] == 0) pending.push(i);
        while (!pending.empty())
        {
            const int i = pending.front(); pending.pop();
            const int x = i % width, y = i / width;
            for (const auto [dx, dy] : {std::pair{0, -1}, {1, 0}, {0, 1}, {-1, 0}})
                if (world.IsInBounds(x + dx, y + dy))
                {
                    const int n = (y + dy) * width + x + dx;
                    if (field[n] > field[i] + 1) { field[n] = field[i] + 1; pending.push(n); }
                }
        }
    };
    spread(wet); spread(depth); spread(road); spread(settlement); spread(ore);
    const auto grassStyle = [&](int x, int y)
    {
        x = std::clamp(x, 0, width - 1); y = std::clamp(y, 0, height - 1);
        const int i = y * width + x;
        const float field = Patch(static_cast<float>(x), static_cast<float>(y), 7, 31);
        if (wet[i] < 4 || cells[i].canopy > 0.35f) return 1; // lush/shaded
        return field > 0.57f && wet[i] > 6 ? 2 : 0; // dry sunlit patches
    };
    const auto flowerScore = [&](int x, int y)
    {
        if (!world.IsInBounds(x, y)) return -1.0f;
        const int i = y * width + x;
        if (world.GetGroundType(x, y) != TileType::GRASS || occupied[i] || settlement[i] < 8 ||
            road[i] < 2 || wet[i] < 2 || cells[i].canopy > 0.12f) return -1.0f;
        return Patch(static_cast<float>(x), static_cast<float>(y), 6, 71) +
            (Hash(x, y, 47) % 1000) * 0.0002f;
    };
    std::vector<int> flowerAnchors(size, -1);
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x)
    {
        const float score = flowerScore(x, y);
        if (score < 0.57f) continue;
        bool maximum = true;
        for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx)
            if ((dx || dy) && flowerScore(x + dx, y + dy) > score) maximum = false;
        if (maximum)
        {
            const int i = y * width + x;
            // Blue flowers favour damp meadows, gold favours dry open patches.
            flowerAnchors[i] = wet[i] <= 4 ? 3 : grassStyle(x, y) == 2 ? 2 :
                static_cast<int>(Hash(x / 6, y / 6, 9) % 2);
        }
    }
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x)
    {
        const int i = y * width + x;
        auto &cell = cells[i];
        const auto h = Hash(x, y, 12);
        const auto ground = world.GetGroundType(x, y);
        cell.wildness = std::clamp((settlement[i] - 3) / 14.0f, 0.0f, 1.0f);
        const int pattern = grassStyle(x, y) + grassStyle(x + 1, y) * 3 +
            grassStyle(x, y + 1) * 9 + grassStyle(x + 1, y + 1) * 27;
        cell.grass = pattern * 2 + static_cast<int>(h % 2);
        const int pathCondition = settlement[i] < 6 ? 0 : cell.wildness > 0.7f ? 2 : 1;
        cell.path = pathCondition * 4 + (settlement[i] < 10 && h % 19 == 0 ? 3 : static_cast<int>(h % 3));
        int waterMask = 0;
        for (int corner = 0; corner < 4; ++corner)
        {
            const int vx = x + corner % 2, vy = y + corner / 2;
            int waterDepth = 10000;
            // Shared vertices inspect the same four cells on both sides of an edge.
            for (int dy = -1; dy <= 0; ++dy) for (int dx = -1; dx <= 0; ++dx)
                if (world.IsInBounds(vx + dx, vy + dy))
                    waterDepth = std::min(waterDepth, depth[(vy + dy) * width + vx + dx]);
            if (waterDepth >= 2) waterMask |= 1 << corner;
        }
        cell.water = waterMask * 3 + static_cast<int>(h % 3);
        cell.stone = ore[i] <= 5 && settlement[i] > 4 ? 9 + h % 3 :
            (settlement[i] < 4 ? 0 : cell.wildness > 0.55f ? 3 : 6) + h % 3;
        const bool land = ground == TileType::GRASS || ground == TileType::DIRT;
        if (land && !world.GetBuildingAt(x, y) && cell.canopy > 0.05f &&
            (ground == TileType::GRASS || (cell.wildness > 0.7f && h % 5 == 0)))
        {
            // A quiet canopy accumulates broad beds; busy edges get sparse leaves.
            const bool dense = cell.canopy * (0.3f + cell.wildness * 0.7f) > 0.22f;
            cell.leaves = (dense ? 20 : 36) + static_cast<int>(Hash(x, y, 15) % 4);
            if (dense && Hash(x, y, 16) % 2) cell.leaves += 4;
        }
        if (ground == TileType::STONE && !occupied[i] && ore[i] <= 4 && h % 3 == 0)
            cell.detail = 28 + static_cast<int>(h % 4);
        if (ground != TileType::GRASS || occupied[i] || road[i] < 1) continue;
        if (cell.wildness > 0.25f && Patch(static_cast<float>(x), static_cast<float>(y), 4, 19) > 0.38f && h % 3 != 0)
            cell.plant = 8 + (grassStyle(x, y) == 2 ? 4 : 0) + static_cast<int>(h % 4);
        if (wet[i] == 1 && settlement[i] > 6 && h % 3 != 0)
            cell.plant = 16 + static_cast<int>(h % 4);
        if (flowerScore(x, y) >= 0)
            for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx)
                if (world.IsInBounds(x + dx, y + dy))
                {
                    const int anchor = flowerAnchors[(y + dy) * width + x + dx];
                    if (anchor >= 0 && (!dx || !dy || h % 3 == 0)) cell.plant = anchor * 2 + static_cast<int>(h % 2);
                }
        if (cell.canopy > 0.22f && cell.plant >= 0 && cell.plant < 8) cell.plant = -1;
        if (ore[i] <= 4 && h % 4 == 0) cell.detail = 28 + static_cast<int>(h % 4);
        else if (wet[i] <= 3 && cell.canopy > 0.1f && h % 4 == 0) cell.detail = 32 + static_cast<int>(h % 4);
    }
}
