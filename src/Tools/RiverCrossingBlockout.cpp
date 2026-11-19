#include "RiverCrossingBlockout.h"

#include "../Content/Import/StarterWorldRuntimeInput.h"

#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    using StarterWorldExport::Snapshot;

    void Rectangle(Snapshot &world, int left, int top, int right, int bottom,
        char terrain, bool preserveWater = false)
    {
        if (left > right || top > bottom || left < 0 || top < 0 ||
            right >= world.width || bottom >= world.height)
            throw std::runtime_error("River Crossing authoring rectangle is outside the map");
        for (int y = top; y <= bottom; ++y)
            for (int x = left; x <= right; ++x)
                if (!preserveWater || world.terrainRows[y][x] != 'W')
                    world.terrainRows[y][x] = terrain;
    }

    void Path(Snapshot &world, std::initializer_list<std::pair<int, int>> points,
        int halfWidth = 1)
    {
        if (points.size() < 2 || halfWidth < 0 || halfWidth > 1)
            throw std::runtime_error("Invalid River Crossing path");
        auto previous = *points.begin();
        for (auto next = points.begin() + 1; next != points.end(); ++next)
        {
            if (previous.first != next->first && previous.second != next->second)
                throw std::runtime_error("River Crossing paths must be axis-aligned");
            const int left = std::max(0, std::min(previous.first, next->first) - halfWidth);
            const int right = std::min(world.width - 1, std::max(previous.first, next->first) + halfWidth);
            const int top = std::max(0, std::min(previous.second, next->second) - halfWidth);
            const int bottom = std::min(world.height - 1, std::max(previous.second, next->second) + halfWidth);
            // Roads never punch another crossing through the channel. Only the
            // two explicitly authored causeway rectangles may replace water.
            Rectangle(world, left, top, right, bottom, 'D', true);
            previous = *next;
        }
    }

    void Home(Snapshot &world, std::string_view id, int x, int y)
    {
        const auto found = std::find_if(world.placements.begin(), world.placements.end(),
            [id](const auto &placement) { return placement.id == id; });
        if (found == world.placements.end())
            throw std::runtime_error("River Crossing is missing a bound placement: " + std::string(id));
        found->position = Position{x, y};
        if (auto *spawn = std::get_if<NpcSpawnDefinition>(&found->legacyDefinition))
            spawn->spawnPosition = found->position;
    }

    void AdditionalResource(Snapshot &world, std::string_view definition, int x, int y)
    {
        world.placements.push_back(StarterWorldExport::AllocatePlacement(
            definition, world.nextPlacementId, Position{x, y}));
        ++world.nextPlacementId;
    }
}

StarterWorldExport::Snapshot RiverCrossingBlockout::Build()
{
    auto world = StarterWorldExport::Capture();
    if (world.width != 100 || world.height != 100)
        throw std::runtime_error("River Crossing requires the current 100 x 100 starter model");
    world.schemaVersion = 2;
    world.terrainRows.assign(world.height, std::string(world.width, 'G'));
    world.playerSpawn = Position{5, 28};

    // River contours, followed by the only two passable land causeways.
    Rectangle(world, 46, 0, 51, 17, 'W');
    Rectangle(world, 48, 18, 54, 35, 'W');
    Rectangle(world, 45, 36, 51, 53, 'W');
    Rectangle(world, 48, 54, 55, 71, 'W');
    Rectangle(world, 45, 72, 52, 99, 'W');
    Rectangle(world, 48, 29, 54, 31, 'D');
    Rectangle(world, 48, 62, 55, 64, 'D');

    // Walkable ground pads reserve future building footprints. They are paving,
    // not walls or elevated floors, so they cannot silently block a service.
    Rectangle(world, 8, 20, 15, 24, 'S');
    Rectangle(world, 29, 22, 37, 27, 'S');
    Rectangle(world, 27, 37, 35, 43, 'S');
    Rectangle(world, 7, 44, 13, 49, 'S');
    Rectangle(world, 65, 40, 75, 48, 'S');
    Rectangle(world, 78, 47, 83, 52, 'S');
    Rectangle(world, 77, 16, 83, 21, 'S');
    Rectangle(world, 82, 57, 96, 71, 'S'); // Quarry working ground.
    Rectangle(world, 25, 66, 29, 70, 'D'); // Optional passive encounter clearing.

    // Three-tile service loops, frontage and branches.
    Path(world, {{0, 28}, {20, 28}, {20, 30}, {62, 30}, {62, 26}, {99, 26}});
    Path(world, {{20, 30}, {20, 42}, {18, 42}, {18, 63}, {48, 63}});
    Path(world, {{40, 30}, {40, 63}});
    Path(world, {{62, 30}, {62, 56}, {74, 56}, {74, 63}, {48, 63}});
    Path(world, {{74, 56}, {82, 56}, {82, 70}});
    Path(world, {{62, 53}, {76, 53}});

    // Short pedestrian links connect the nominated approach tiles.
    Path(world, {{14, 25}, {14, 28}}, 0);
    Path(world, {{33, 27}, {33, 30}}, 0);
    Path(world, {{80, 24}, {80, 26}}, 0);
    Path(world, {{16, 51}, {18, 51}}, 0);
    Path(world, {{68, 50}, {68, 53}}, 0);
    Path(world, {{73, 50}, {73, 53}}, 0);
    Path(world, {{80, 54}, {80, 56}}, 0);

    // A narrow optional trail leaves the working quarry before the encounters.
    Path(world, {{82, 70}, {82, 75}, {84, 75}, {84, 82}, {91, 82}, {91, 80}}, 0);
    Path(world, {{91, 82}, {93, 82}, {93, 92}}, 0);

    Home(world, "station.bank.001", 33, 27);
    Home(world, "station.furnace.001", 68, 50);
    Home(world, "station.crafting_table.001", 73, 50);
    Home(world, "npc.development_guide.001", 14, 25);
    Home(world, "npc.elia_reed.001", 35, 27);
    Home(world, "npc.nessa_thorne.001", 16, 51);
    Home(world, "npc.torren_ash.001", 70, 51);
    Home(world, "npc.brann_deepwell.001", 80, 54);
    Home(world, "npc.sera_flint.001", 80, 24);
    Home(world, "monster.passive_development.001", 27, 68);
    Home(world, "monster.aggressive_development.001", 84, 82);
    Home(world, "monster.durable_development.001", 91, 80);
    Home(world, "monster.elite_development.001", 93, 92);
    Home(world, "resource.normal_tree.001", 8, 55);
    Home(world, "resource.oak_tree.001", 5, 72);
    Home(world, "resource.willow_tree.001", 38, 74);
    Home(world, "resource.copper_rock.001", 84, 60);
    Home(world, "resource.tin_rock.001", 87, 64);
    Home(world, "resource.iron_rock.001", 91, 68);
    Home(world, "resource.coal_rock.001", 95, 69);

    AdditionalResource(world, "resource.normal_tree", 12, 58);
    AdditionalResource(world, "resource.normal_tree", 7, 63);
    AdditionalResource(world, "resource.normal_tree", 15, 66);
    AdditionalResource(world, "resource.normal_tree", 24, 51);
    AdditionalResource(world, "resource.normal_tree", 28, 55);
    AdditionalResource(world, "resource.normal_tree", 26, 61);
    AdditionalResource(world, "resource.normal_tree", 32, 65);
    AdditionalResource(world, "resource.oak_tree", 14, 74);
    AdditionalResource(world, "resource.oak_tree", 25, 73);
    AdditionalResource(world, "resource.copper_rock", 88, 58);
    AdditionalResource(world, "resource.tin_rock", 91, 61);

    // Author the entire private candidate before validating it: temporary
    // intermediate terrain/old homes need not be individually publishable.
    return StarterWorldImport::ValidateForRuntime(world);
}
