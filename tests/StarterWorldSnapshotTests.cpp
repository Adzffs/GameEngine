#include "TestSupport.h"

#include "../src/Content/Export/StarterWorldSnapshot.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/NPC/NPC.h"
#include "../src/NPC/NpcSpawnDatabase.h"
#include "../src/Pathfinding/Pathfinder.h"
#include "../src/World/World.h"

#include <algorithm>
#include <array>
#include <locale>
#include <map>
#include <set>
#include <stdexcept>

namespace
{
    using namespace StarterWorldExport;

    TileType DecodeTerrain(char symbol)
    {
        switch (symbol)
        {
        case 'G': return TileType::GRASS;
        case 'D': return TileType::DIRT;
        case 'S': return TileType::STONE;
        case 'W': return TileType::WATER;
        default: throw std::runtime_error("unexpected fixture terrain");
        }
    }

    bool SameSpawn(const NpcSpawnDefinition &left, const NpcSpawnDefinition &right)
    {
        return left.spawnId == right.spawnId && left.npcType == right.npcType &&
            left.spawnPosition.GetX() == right.spawnPosition.GetX() &&
            left.spawnPosition.GetY() == right.spawnPosition.GetY() &&
            left.wanderRadius == right.wanderRadius &&
            left.wanderIntervalTicks == right.wanderIntervalTicks &&
            left.maximumActiveCount == right.maximumActiveCount && left.respawns == right.respawns;
    }

    const Entity *FindActor(const World &world, NpcSpawnId spawnId)
    {
        for (const auto &entity : world.GetEntities())
        {
            if (const auto *npc = dynamic_cast<const NPC *>(entity.get()))
            {
                if (npc->GetNpcSpawnId() == spawnId)
                    return npc;
            }
            if (const auto *monster = dynamic_cast<const Monster *>(entity.get()))
            {
                if (monster->GetNpcSpawnId() == spawnId)
                    return monster;
            }
        }
        return nullptr;
    }

    void CheckTerrainAndObjects(TestContext &test, const Snapshot &snapshot, World &world)
    {
        test.ExpectEqual(snapshot.width, world.GetMap().GetWidth(), "Snapshot width matches actual World");
        test.ExpectEqual(snapshot.height, world.GetMap().GetHeight(), "Snapshot height matches actual World");
        test.ExpectEqual(snapshot.width, 100, "Recorded starter width is 100 tiles");
        test.ExpectEqual(snapshot.height, 100, "Recorded starter height is 100 tiles");
        test.ExpectEqual(snapshot.placements.size(), std::size_t{20}, "Full starter registry has 20 placements");

        Map reconstructed(snapshot.width, snapshot.height);
        std::map<char, int> groundCounts;
        for (int y = 0; y < snapshot.height; ++y)
            for (int x = 0; x < snapshot.width; ++x)
            {
                const char ground = snapshot.terrainRows.at(y).at(x);
                ++groundCounts[ground];
                reconstructed.SetTileType(x, y, DecodeTerrain(ground));
            }
        test.ExpectEqual(groundCounts['G'], 9791, "Ground grass count retains the complete empty region");
        test.ExpectEqual(groundCounts['D'], 100, "Ordered overlays retain dirt count");
        test.ExpectEqual(groundCounts['S'], 108, "Ordered overlays retain stone count");
        test.ExpectEqual(groundCounts['W'], 1, "Constructor pond survives stone overlay");
        test.Expect(snapshot.terrainRows.at(10).at(10) == 'W', "Pond is at (10,10)");

        std::vector<std::pair<int, int>> objectOrder;
        std::size_t resources = 0;
        std::size_t stations = 0;
        for (const auto &placement : snapshot.placements)
        {
            const int x = placement.position.GetX();
            const int y = placement.position.GetY();
            if (const auto *type = std::get_if<ResourceType>(&placement.legacyDefinition))
            {
                ++resources;
                const auto *actual = world.GetResourceAt(x, y);
                test.Expect(actual != nullptr, "Every exported resource exists in actual World: " + placement.id);
                if (actual != nullptr)
                {
                    test.Expect(actual->GetResourceType() == *type, "Resource definition matches: " + placement.id);
                    objectOrder.emplace_back(placement.order, actual->GetID());
                }
                const bool tree = *type == ResourceType::NORMAL_TREE ||
                    *type == ResourceType::OAK_TREE || *type == ResourceType::WILLOW_TREE;
                test.Expect(snapshot.terrainRows.at(y).at(x) == (tree ? 'G' : 'S'),
                    "Occupied resource ground is preserved: " + placement.id);
                reconstructed.SetTileType(x, y, TileType::TREE);
            }
            if (const auto *type = std::get_if<StationType>(&placement.legacyDefinition))
            {
                ++stations;
                const auto *actual = world.GetStationAt(x, y);
                test.Expect(actual != nullptr, "Every exported station exists in actual World: " + placement.id);
                if (actual != nullptr)
                {
                    test.Expect(actual->GetStationType() == *type, "Station definition matches: " + placement.id);
                    objectOrder.emplace_back(placement.order, actual->GetID());
                }
                test.Expect(snapshot.terrainRows.at(y).at(x) == (*type == StationType::BANK ? 'D' : 'S'),
                    "Occupied station ground is preserved: " + placement.id);
                reconstructed.SetTileType(x, y, TileType::STATION);
            }
        }
        test.ExpectEqual(resources, world.GetResources().size(), "Export includes all runtime resources");
        test.ExpectEqual(resources, std::size_t{7}, "Recorded resource count is seven");
        test.ExpectEqual(stations, world.GetStations().size(), "Export includes all runtime stations");
        test.ExpectEqual(stations, std::size_t{3}, "Recorded station count is three");
        std::sort(objectOrder.begin(), objectOrder.end());
        for (std::size_t i = 1; i < objectOrder.size(); ++i)
            test.Expect(objectOrder[i - 1].second < objectOrder[i].second,
                "Lifecycle order preserves the shared resource/station ID sequence");

        bool sameTiles = true;
        bool samePassability = true;
        int blocked = 0;
        for (int y = 0; y < snapshot.height; ++y)
            for (int x = 0; x < snapshot.width; ++x)
            {
                sameTiles &= reconstructed.GetTileType(x, y) == world.GetMap().GetTileType(x, y);
                samePassability &= reconstructed.IsValidPosition(x, y) == world.GetMap().IsValidPosition(x, y);
                if (!reconstructed.IsValidPosition(x, y))
                    ++blocked;
            }
        test.Expect(sameTiles, "All 10,000 effective tiles match the real World");
        test.Expect(samePassability, "All 10,000 collision observations match the real World");
        test.ExpectEqual(blocked, 11, "Pond and ten objects are blocked; actors do not change map occupancy");

        // Use the actual pathfinder. A partial best-effort path is not success.
        Pathfinder pathfinder;
        const std::array<std::pair<int, int>, 4> neighbors{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
        for (const auto &placement : snapshot.placements)
        {
            if (placement.kind != PlacementKind::Resource && placement.kind != PlacementKind::Station)
                continue;
            bool foundApproach = false;
            for (const auto &[dx, dy] : neighbors)
            {
                const int x = placement.position.GetX() + dx;
                const int y = placement.position.GetY() + dy;
                if (!reconstructed.IsValidPosition(x, y))
                    continue;
                const auto path = pathfinder.FindPath(reconstructed,
                    snapshot.playerSpawn.GetX(), snapshot.playerSpawn.GetY(), x, y);
                if (!path.empty() && path.back().x == x && path.back().y == y)
                {
                    foundApproach = true;
                    break;
                }
            }
            test.Expect(foundApproach, "Authoritative pathfinder reaches a cardinal approach: " + placement.id);
        }
    }

    void CheckActors(TestContext &test, const Snapshot &snapshot, const World &world, bool playerRegistered)
    {
        std::map<InitializationStage, std::vector<std::pair<int, int>>> stageOrder;
        for (const auto &placement : snapshot.placements)
        {
            const auto *spawn = std::get_if<NpcSpawnDefinition>(&placement.legacyDefinition);
            if (spawn == nullptr)
                continue;
            test.Expect(world.GetNpcSpawnManager().HasSpawn(spawn->spawnId),
                "Every spawn is registered before any player: " + placement.id);
            const bool expectedActive = placement.stage == InitializationStage::WorldStart || playerRegistered;
            test.ExpectEqual(world.GetNpcSpawnManager().GetActiveCount(spawn->spawnId), expectedActive ? 1 : 0,
                "Stage determines active count: " + placement.id);
            const Entity *actor = FindActor(world, spawn->spawnId);
            test.Expect((actor != nullptr) == expectedActive, "Stage determines actor presence: " + placement.id);
            if (actor == nullptr)
                continue;
            stageOrder[placement.stage].emplace_back(placement.order, actor->GetID());
            test.Expect(actor->GetPosition().GetX() == placement.position.GetX() &&
                actor->GetPosition().GetY() == placement.position.GetY(), "Initial actor position matches: " + placement.id);
            if (const auto *npc = dynamic_cast<const NPC *>(actor))
            {
                test.Expect(placement.kind == PlacementKind::NpcSpawn && npc->GetNpcType() == spawn->npcType,
                    "Friendly type matches: " + placement.id);
            }
            if (const auto *monster = dynamic_cast<const Monster *>(actor))
            {
                test.Expect(placement.kind == PlacementKind::MonsterSpawn && monster->GetNpcType() == spawn->npcType,
                    "Monster type matches: " + placement.id);
                test.ExpectEqual(monster->GetWanderRadius(), spawn->wanderRadius, "Monster wander radius matches");
                test.ExpectEqual(monster->GetWanderIntervalTicks(), spawn->wanderIntervalTicks, "Monster wander interval matches");
                test.Expect(monster->HasRespawnDefinition() == spawn->respawns, "Monster respawn behavior matches");
            }
        }
        for (auto &[stage, order] : stageOrder)
        {
            std::sort(order.begin(), order.end());
            for (std::size_t i = 1; i < order.size(); ++i)
                test.Expect(order[i - 1].second < order[i].second,
                    "Actor creation order follows lifecycle keys rather than canonical ID order");
        }
    }

    int CheckLifecycle(TestContext &test, const Snapshot &snapshot, World &world)
    {
        test.ExpectEqual(world.GetEntities().size(), std::size_t{3}, "Exactly three actors exist before player registration");
        CheckActors(test, snapshot, world, false);
        const int playerId = world.CreatePlayer();
        const auto *player = world.GetEntityByID(playerId);
        test.Expect(player != nullptr, "Player creation succeeds");
        if (player != nullptr)
            test.Expect(player->GetPosition().GetX() == snapshot.playerSpawn.GetX() &&
                player->GetPosition().GetY() == snapshot.playerSpawn.GetY(), "Exported spawn matches direct CreatePlayer behavior");
        test.ExpectEqual(world.GetEntities().size(), std::size_t{11}, "First player activates the seven delayed actors");
        CheckActors(test, snapshot, world, true);
        for (const auto &placement : snapshot.placements)
        {
            const auto *spawn = std::get_if<NpcSpawnDefinition>(&placement.legacyDefinition);
            if (spawn == nullptr)
                continue;
            const auto *actor = FindActor(world, spawn->spawnId);
            if (actor != nullptr)
                test.Expect(placement.stage == InitializationStage::WorldStart ? actor->GetID() < playerId : actor->GetID() > playerId,
                    "Player ID allocation separates initial and delayed actors: " + placement.id);
        }
        const int secondPlayerId = world.CreatePlayer();
        test.Expect(secondPlayerId > playerId, "A second player can register");
        test.ExpectEqual(world.GetEntities().size(), std::size_t{12}, "Additional players do not duplicate starter actors");
        CheckActors(test, snapshot, world, true);
        test.ExpectEqual(world.GetCurrentTick(), 0, "Capturing and checking initial content does not tick simulation");

        std::set<NpcSpawnId> monsterSubset;
        for (const auto &spawn : NpcSpawnDatabase::GetStarterMonsterSpawns())
        {
            const auto *canonical = NpcSpawnDatabase::TryGet(spawn.spawnId);
            test.Expect(canonical != nullptr && SameSpawn(spawn, *canonical),
                "Duplicated monster spawn table agrees with canonical spawn definitions");
            test.Expect(monsterSubset.insert(spawn.spawnId).second, "Monster subset has no duplicate spawn");
        }
        test.ExpectEqual(monsterSubset.size(), std::size_t{4}, "All four monster spawns are covered");
        return playerId;
    }

    void CheckStationInteraction(TestContext &test, const Snapshot &snapshot, World &world, int playerId)
    {
        const auto placement = std::find_if(snapshot.placements.begin(), snapshot.placements.end(), [](const auto &entry) {
            const auto *type = std::get_if<StationType>(&entry.legacyDefinition);
            return type != nullptr && *type == StationType::FURNACE;
        });
        test.Expect(placement != snapshot.placements.end(), "Export includes the station used by interaction smoke test");
        if (placement == snapshot.placements.end())
            return;
        const auto *station = world.GetStationAt(placement->position.GetX(), placement->position.GetY());
        test.Expect(station != nullptr, "Exported furnace position resolves to a real station");
        if (station == nullptr)
            return;

        const auto initialJson = SerializeCanonical(snapshot);
        world.QueueStationInteraction(playerId, station->GetID());
        world.QueueMovementDestination(MovementDestinationRequest(
            playerId, placement->position.GetX(), placement->position.GetY()));
        bool opened = false;
        StationType openedType = StationType::NONE;
        // Route from the original player spawn through the actual movement and
        // interaction systems. No teleport, private seam, or wall-clock wait.
        for (int tick = 0; tick < 64 && !opened; ++tick)
        {
            world.Update();
            opened = world.ConsumeOpenedStation(playerId, openedType);
        }
        test.Expect(opened && openedType == StationType::FURNACE,
            "Player reaches and opens exported furnace through real movement/interaction within 64 ticks");
        test.Expect(world.HasActiveStation(playerId), "Successful interaction leaves an active station session");
        const auto *player = world.GetEntityByID(playerId);
        test.Expect(player != nullptr &&
            (player->GetPosition().GetX() != snapshot.playerSpawn.GetX() ||
             player->GetPosition().GetY() != snapshot.playerSpawn.GetY()),
            "Interaction moved the player from the initial spawn");
        test.Expect(SerializeCanonical(Capture()) == initialJson,
            "Movement, wandering, ticking and station sessions do not alter initial-content export");
    }

    struct GroupedNumbers : std::numpunct<char>
    {
        char do_thousands_sep() const override { return ','; }
        std::string do_grouping() const override { return "\1"; }
    };

    void CheckCanonicalWriter(TestContext &test, const Snapshot &snapshot)
    {
        const auto json = SerializeCanonical(snapshot);
        test.Expect(json == SerializeCanonical(Capture()), "Repeated capture is byte-for-byte deterministic");
        auto shuffled = snapshot;
        std::reverse(shuffled.placements.begin(), shuffled.placements.end());
        const auto firstId = shuffled.placements.front().id;
        test.Expect(json == SerializeCanonical(shuffled), "Canonical output is independent of storage order");
        test.ExpectEqual(shuffled.placements.front().id, firstId, "Writer does not mutate caller's placement order");
        test.Expect(!json.empty() && json.back() == '\n' && json.find('\r') == std::string::npos,
            "Canonical output has LF line endings and a final newline");
        test.Expect(json.find("\"format\": \"embervale.starter-world-snapshot\"") != std::string::npos,
            "Export declares its own format rather than implying runtime import support");

        const auto previousLocale = std::locale();
        std::locale::global(std::locale(previousLocale, new GroupedNumbers));
        const auto localizedJson = SerializeCanonical(snapshot);
        std::locale::global(previousLocale);
        test.Expect(localizedJson == json, "Canonical numbers do not depend on the process locale");

        std::vector<std::string> ids;
        for (const auto &placement : snapshot.placements)
            ids.push_back(placement.id);
        std::sort(ids.begin(), ids.end());
        std::size_t previousOffset = 0;
        for (const auto &id : ids)
        {
            const auto offset = json.find("\"id\": \"" + id + "\"");
            test.Expect(offset != std::string::npos && offset > previousOffset,
                "Serialized placement IDs follow lexical canonical order: " + id);
            previousOffset = offset;
        }

        auto reject = [&](auto mutate, const std::string &message) {
            auto invalid = snapshot;
            mutate(invalid);
            bool rejected = false;
            try { (void)SerializeCanonical(invalid); }
            catch (const std::runtime_error &) { rejected = true; }
            test.Expect(rejected, message);
        };
        reject([](auto &value) { value.width = 0; }, "Writer rejects empty dimensions");
        reject([](auto &value) { value.width = 1025; }, "Writer rejects dimensions above supported bounds");
        reject([](auto &value) { value.terrainRows.pop_back(); }, "Writer rejects missing rows");
        reject([](auto &value) { value.terrainRows[0].pop_back(); }, "Writer rejects short rows");
        reject([](auto &value) { value.terrainRows[0][0] = 'T'; }, "Writer rejects occupancy encoded as base terrain");
        reject([](auto &value) { value.placements[1].id = value.placements[0].id; }, "Writer rejects duplicate stable IDs");
        reject([](auto &value) { value.placements[1].order = value.placements[0].order; }, "Writer rejects duplicate stage/order keys");
        reject([](auto &value) { value.placements[0].position.SetPosition(-1, 0); }, "Writer rejects out-of-bounds placements");
        reject([](auto &value) { value.playerSpawn.SetPosition(value.width, 0); }, "Writer rejects out-of-bounds player spawn");
        reject([](auto &value) { value.placements[0].kind = static_cast<PlacementKind>(999); }, "Writer rejects unknown placement kinds");
        reject([](auto &value) { value.placements[0].definitionId = "npc.missing"; }, "Writer rejects mismatched definition bindings");
        reject([](auto &value) { value.placements[0].id = "bad\nidentity"; }, "Writer rejects invalid stable identity characters");
    }
}

int main()
{
    TestContext test;
    try
    {
        const auto snapshot = StarterWorldExport::Capture();
        World world(12345U, 7331U, 9001U);
        CheckTerrainAndObjects(test, snapshot, world);
        const int playerId = CheckLifecycle(test, snapshot, world);
        CheckTerrainAndObjects(test, snapshot, world);
        CheckCanonicalWriter(test, snapshot);
        CheckStationInteraction(test, snapshot, world, playerId);
    }
    catch (const std::exception &error)
    {
        test.Expect(false, std::string("Unexpected snapshot exception: ") + error.what());
    }
    return test.Finish();
}
