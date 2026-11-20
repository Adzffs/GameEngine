#include "StarterWorldSnapshotInput.h"
#include "../Json/BoundedJson.h"

#include <algorithm>
#include <charconv>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace StarterWorldExport
{
    namespace
    {
        using Json = BoundedJson::Value;

        [[noreturn]] void Invalid(const std::string &reason)
        {
            throw std::runtime_error("starter world input: " + reason);
        }

        const Json &Field(const Json &object, const std::string &name)
        {
            if (object.type != Json::Type::Object)
                Invalid("expected object containing " + name);
            const auto found = object.object.find(name);
            if (found == object.object.end())
                Invalid("missing field: " + name);
            return found->second;
        }

        void Keys(const Json &object, std::initializer_list<const char *> expected)
        {
            if (object.type != Json::Type::Object || object.object.size() != expected.size())
                Invalid("unexpected object fields");
            for (const char *name : expected)
                Field(object, name);
        }

        const std::string &String(const Json &object, const char *name)
        {
            const Json &value = Field(object, name);
            if (value.type != Json::Type::String)
                Invalid(std::string("expected string: ") + name);
            return value.string;
        }

        int Number(const Json &object, const char *name)
        {
            const Json &value = Field(object, name);
            if (value.type != Json::Type::Number)
                Invalid(std::string("expected integer: ") + name);
            return value.number;
        }

        bool Boolean(const Json &object, const char *name)
        {
            const Json &value = Field(object, name);
            if (value.type != Json::Type::Boolean)
                Invalid(std::string("expected boolean: ") + name);
            return value.boolean;
        }

        void Expect(const std::string &actual, const char *expected)
        {
            if (actual != expected)
                Invalid(std::string("unsupported value: ") + actual);
        }

        Position ReadPosition(const Json &value)
        {
            Keys(value, {"x", "y", "surface_id"});
            Expect(String(value, "surface_id"), "ground");
            return Position{Number(value, "x"), Number(value, "y")};
        }

    }

    Snapshot ParseSnapshot(std::string_view input)
    {
        if (input.size() > MaximumSnapshotInputBytes) Invalid("input exceeds 2 MiB");
        const Json root = BoundedJson::Parse(input, MaximumSnapshotInputBytes, "starter world input");
        if (root.type != Json::Type::Object)
            Invalid("unexpected object fields");
        const int schemaVersion = Number(root, "schema_version");
        if (schemaVersion == 1)
            Keys(root, {"format", "schema_version", "region_id", "revision", "size",
                "surfaces", "terrain", "player_spawn", "placements", "region_exits"});
        else if (schemaVersion == 2 || schemaVersion == 3)
            Keys(root, {"format", "schema_version", "region_id", "revision", "size",
                "surfaces", "terrain", "player_spawn", "placements", "region_exits", "next_placement_id"});
        else if (schemaVersion == 4)
            Keys(root, {"format", "schema_version", "region_id", "revision", "size",
                "surfaces", "terrain", "player_spawn", "placements", "region_exits", "next_placement_id", "buildings"});
        else
            Invalid("unsupported schema or revision");
        Expect(String(root, "format"), "embervale.starter-world-snapshot");
        if (Number(root, "revision") != 1)
            Invalid("unsupported schema or revision");
        Expect(String(root, "region_id"), "embervale.development_starter");

        const Json &size = Field(root, "size");
        Keys(size, {"width", "height"});
        const int width = Number(size, "width");
        const int height = Number(size, "height");
        if (width < 1 || width > 1024 || height < 1 || height > 1024)
            Invalid("dimensions out of range");

        const Json &surfaces = Field(root, "surfaces");
        if (surfaces.type != Json::Type::Array || surfaces.array.size() != 1)
            Invalid("only one ground surface is supported");
        Keys(surfaces.array.front(), {"id", "elevation"});
        Expect(String(surfaces.array.front(), "id"), "ground");
        if (Number(surfaces.array.front(), "elevation") != 0)
            Invalid("elevation is unsupported");

        const Json &terrain = Field(root, "terrain");
        Keys(terrain, {"encoding", "legend", "rows"});
        Expect(String(terrain, "encoding"), "rows");
        const Json &legend = Field(terrain, "legend");
        Keys(legend, {"G", "D", "S", "W"});
        Expect(String(legend, "G"), "terrain.grass");
        Expect(String(legend, "D"), "terrain.dirt");
        Expect(String(legend, "S"), "terrain.stone");
        Expect(String(legend, "W"), "terrain.water");
        const Json &rows = Field(terrain, "rows");
        if (rows.type != Json::Type::Array || rows.array.size() != static_cast<std::size_t>(height))
            Invalid("terrain row count mismatch");

        Snapshot snapshot;
        snapshot.schemaVersion = schemaVersion;
        snapshot.nextPlacementId = schemaVersion >= 2 ? Number(root, "next_placement_id") : 1;
        if (snapshot.nextPlacementId < 1 || snapshot.nextPlacementId > MaximumNextPlacementId)
            Invalid("invalid placement allocation counter");
        snapshot.width = width;
        snapshot.height = height;
        for (const Json &row : rows.array)
        {
            if (row.type != Json::Type::String ||
                row.string.size() != static_cast<std::size_t>(width) ||
                row.string.find_first_not_of("GDSW") != std::string::npos)
                Invalid("invalid terrain row");
            snapshot.terrainRows.push_back(row.string);
        }
        snapshot.playerSpawn = ReadPosition(Field(root, "player_spawn"));
        if (schemaVersion >= 4)
        {
            const auto &buildings = Field(root, "buildings");
            if (buildings.type != Json::Type::Array || buildings.array.size() > 64)
                Invalid("invalid building array");
            for (const auto &item : buildings.array)
            {
                Keys(item, {"id", "definition_id", "x", "y", "width", "height", "door_offset"});
                Expect(String(item, "definition_id"), "building.timber_stone");
                snapshot.buildings.push_back({String(item, "id"), Number(item, "x"), Number(item, "y"),
                    Number(item, "width"), Number(item, "height"), Number(item, "door_offset")});
            }
        }

        const Json &exits = Field(root, "region_exits");
        if (exits.type != Json::Type::Array || !exits.array.empty())
            Invalid("region exits are not supported");

        const Json &placements = Field(root, "placements");
        if (placements.type != Json::Type::Array || placements.array.size() > 4096)
            Invalid("invalid placement array");

        // v1 retains its historical exact registry. v2 adds bounded object
        // identities; v3 permits bounded spawn settings. Compiled actor
        // bindings and lifecycle remain mandatory in every version.
        const Snapshot baseline = Capture();
        std::map<std::string, Placement> known;
        for (const auto &placement : baseline.placements)
            known.emplace(placement.id, placement);

        std::set<std::string> seen;
        std::set<int> allocatedSequences;
        for (const Json &item : placements.array)
        {
            if (item.type != Json::Type::Object)
                Invalid("placement must be an object");
            const std::string &id = String(item, "id");
            if (!seen.insert(id).second)
                Invalid("unknown or duplicate placement ID: " + id);
            const auto found = known.find(id);
            if (found == known.end() && schemaVersion == 1)
                Invalid("unknown or duplicate placement ID: " + id);
            Placement placement = found != known.end() ? found->second :
                BindAuthoredPlacement(id, String(item, "definition_id"), Position{0, 0});
            if (found != known.end())
                known.erase(found);
            else
            {
                const int sequence = AuthoredPlacementSequence(id);
                if (sequence >= snapshot.nextPlacementId ||
                    !allocatedSequences.insert(sequence).second)
                    Invalid("unreserved or duplicate authored allocation: " + id);
            }
            const bool isSpawn = std::holds_alternative<NpcSpawnDefinition>(placement.legacyDefinition);
            if (isSpawn)
                Keys(item, {"id", "kind", "definition_id", "initialization", "position", "spawn"});
            else
                Keys(item, {"id", "kind", "definition_id", "initialization", "position"});
            const std::string expectedKind =
                placement.kind == PlacementKind::Resource ? "resource" :
                placement.kind == PlacementKind::Station ? "station" :
                placement.kind == PlacementKind::NpcSpawn ? "npc_spawn" : "monster_spawn";
            if (String(item, "kind") != expectedKind ||
                String(item, "definition_id") != placement.definitionId)
                Invalid("placement definition mismatch: " + id);

            const Json &init = Field(item, "initialization");
            Keys(init, {"stage", "order"});
            const std::string expectedStage =
                placement.stage == InitializationStage::WorldStart ?
                "world_start" : "first_player_registration";
            if (String(init, "stage") != expectedStage ||
                Number(init, "order") != placement.order)
                Invalid("placement lifecycle mismatch: " + id);
            placement.position = ReadPosition(Field(item, "position"));
            if (isSpawn)
            {
                const Json &spawnJson = Field(item, "spawn");
                if (schemaVersion >= 3)
                    Keys(spawnJson, {"spawn_id", "wander_radius", "wander_interval_ticks",
                        "maximum_active_count", "respawns", "respawn_delay_ticks"});
                else
                    Keys(spawnJson, {"spawn_id", "wander_radius", "wander_interval_ticks",
                        "maximum_active_count", "respawns"});
                auto &spawn = std::get<NpcSpawnDefinition>(placement.legacyDefinition);
                // Capture's explicit spawn registry binds each stable placement ID.
                // Match the reviewed baseline spawn token by reading the canonical
                // baseline representation rather than trusting a supplied enum.
                if (schemaVersion < 3 && (Number(spawnJson, "wander_radius") != spawn.wanderRadius ||
                    Number(spawnJson, "wander_interval_ticks") != spawn.wanderIntervalTicks ||
                    Number(spawnJson, "maximum_active_count") != spawn.maximumActiveCount ||
                    Boolean(spawnJson, "respawns") != spawn.respawns))
                    Invalid("unsupported spawn configuration change: " + id);
                if (schemaVersion >= 3)
                {
                    spawn.wanderRadius = Number(spawnJson, "wander_radius");
                    spawn.wanderIntervalTicks = Number(spawnJson, "wander_interval_ticks");
                    spawn.maximumActiveCount = Number(spawnJson, "maximum_active_count");
                    spawn.respawns = Boolean(spawnJson, "respawns");
                    spawn.respawnDelayTicks = Number(spawnJson, "respawn_delay_ticks");
                }
                const std::string &token = String(spawnJson, "spawn_id");
                if (token.empty() || token.size() > 128 ||
                    !std::all_of(token.begin(), token.end(), [](unsigned char c) {
                        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                            c == '.' || c == '_' || c == '-';
                    }))
                    Invalid("invalid spawn reference: " + id);
                // The stable token is independently checked against canonical
                // output below, after the position has been rebound.
                spawn.spawnPosition = placement.position;
            }
            snapshot.placements.push_back(std::move(placement));
        }
        if (schemaVersion == 1 && !known.empty())
            Invalid("missing compiled placement identities");
        for (const auto &[id, placement] : known)
            if (placement.kind == PlacementKind::NpcSpawn || placement.kind == PlacementKind::MonsterSpawn)
                Invalid("missing compiled actor identities: " + id);

        // Reuse the writer's structural and typed validation, then compare
        // the caller's spawn references with canonical registry bindings.
        const std::string canonical = SerializeCanonical(snapshot);
        const Json checked = BoundedJson::Parse(canonical, MaximumSnapshotInputBytes, "starter world input");
        const auto &checkedPlacements = Field(checked, "placements").array;
        for (const Json &item : placements.array)
        {
            if (item.object.find("spawn") == item.object.end())
                continue;
            const std::string &id = String(item, "id");
            const auto matched = std::find_if(checkedPlacements.begin(), checkedPlacements.end(),
                [&id](const Json &candidate) { return String(candidate, "id") == id; });
            if (matched == checkedPlacements.end() ||
                String(Field(item, "spawn"), "spawn_id") !=
                    String(Field(*matched, "spawn"), "spawn_id"))
                Invalid("spawn reference mismatch: " + id);
        }
        return snapshot;
    }
}
