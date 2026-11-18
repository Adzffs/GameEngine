#include "StarterWorldSnapshot.h"

#include "../../NPC/NpcDefinitionDatabase.h"
#include "../../NPC/NpcSpawnDatabase.h"
#include "../../World/Development/DevelopmentWorldContent.h"
#include "../../World/Map.h"

#include <algorithm>
#include <array>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace StarterWorldExport
{
    namespace
    {
        struct ResourceIdentity
        {
            ResourceType type;
            const char *id;
            const char *definitionId;
        };

        constexpr std::array ResourceIdentities{
            ResourceIdentity{ResourceType::NORMAL_TREE, "resource.normal_tree.001", "resource.normal_tree"},
            ResourceIdentity{ResourceType::OAK_TREE, "resource.oak_tree.001", "resource.oak_tree"},
            ResourceIdentity{ResourceType::WILLOW_TREE, "resource.willow_tree.001", "resource.willow_tree"},
            ResourceIdentity{ResourceType::COPPER_ROCK, "resource.copper_rock.001", "resource.copper_rock"},
            ResourceIdentity{ResourceType::TIN_ROCK, "resource.tin_rock.001", "resource.tin_rock"},
            ResourceIdentity{ResourceType::IRON_ROCK, "resource.iron_rock.001", "resource.iron_rock"},
            ResourceIdentity{ResourceType::COAL_ROCK, "resource.coal_rock.001", "resource.coal_rock"}};

        struct StationIdentity
        {
            StationType type;
            const char *id;
            const char *definitionId;
        };

        constexpr std::array StationIdentities{
            StationIdentity{StationType::FURNACE, "station.furnace.001", "station.furnace"},
            StationIdentity{StationType::CRAFTING_TABLE, "station.crafting_table.001", "station.crafting_table"},
            StationIdentity{StationType::BANK, "station.bank.001", "station.bank"}};

        struct SpawnIdentity
        {
            NpcSpawnId spawnId;
            NpcType npcType;
            const char *id;
            const char *definitionId;
            const char *spawnReference;
            PlacementKind kind;
            InitializationStage stage;
            int order;
        };

        // Explicit legacy bridge. Positions and runtime entity numbers are not
        // identities. A new/repeated/deleted legacy placement needs a reviewed
        // registry update rather than silently inventing or recycling an ID.
        constexpr std::array SpawnIdentities{
            SpawnIdentity{NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN, NpcType::DEVELOPMENT_GUIDE,
                "npc.development_guide.001", "npc.development_guide", "spawn.development_guide",
                PlacementKind::NpcSpawn, InitializationStage::WorldStart, 0},
            SpawnIdentity{NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN, NpcType::PASSIVE_DEVELOPMENT_MONSTER,
                "monster.passive_development.001", "npc.passive_development_monster", "spawn.passive_development",
                PlacementKind::MonsterSpawn, InitializationStage::WorldStart, 1},
            SpawnIdentity{NpcSpawnId::AGGRESSIVE_DEVELOPMENT_SPAWN, NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER,
                "monster.aggressive_development.001", "npc.aggressive_development_monster", "spawn.aggressive_development",
                PlacementKind::MonsterSpawn, InitializationStage::WorldStart, 2},
            SpawnIdentity{NpcSpawnId::DURABLE_DEVELOPMENT_SPAWN, NpcType::DURABLE_DEVELOPMENT_MONSTER,
                "monster.durable_development.001", "npc.durable_development_monster", "spawn.durable_development",
                PlacementKind::MonsterSpawn, InitializationStage::FirstPlayerRegistration, 0},
            SpawnIdentity{NpcSpawnId::ELITE_DEVELOPMENT_SPAWN, NpcType::ELITE_DEVELOPMENT_MONSTER,
                "monster.elite_development.001", "npc.elite_development_monster", "spawn.elite_development",
                PlacementKind::MonsterSpawn, InitializationStage::FirstPlayerRegistration, 1},
            SpawnIdentity{NpcSpawnId::TORREN_ASH_SPAWN, NpcType::TORREN_ASH,
                "npc.torren_ash.001", "npc.torren_ash", "spawn.torren_ash",
                PlacementKind::NpcSpawn, InitializationStage::FirstPlayerRegistration, 2},
            SpawnIdentity{NpcSpawnId::ELIA_REED_SPAWN, NpcType::ELIA_REED,
                "npc.elia_reed.001", "npc.elia_reed", "spawn.elia_reed",
                PlacementKind::NpcSpawn, InitializationStage::FirstPlayerRegistration, 3},
            SpawnIdentity{NpcSpawnId::NESSA_THORNE_SPAWN, NpcType::NESSA_THORNE,
                "npc.nessa_thorne.001", "npc.nessa_thorne", "spawn.nessa_thorne",
                PlacementKind::NpcSpawn, InitializationStage::FirstPlayerRegistration, 4},
            SpawnIdentity{NpcSpawnId::BRANN_DEEPWELL_SPAWN, NpcType::BRANN_DEEPWELL,
                "npc.brann_deepwell.001", "npc.brann_deepwell", "spawn.brann_deepwell",
                PlacementKind::NpcSpawn, InitializationStage::FirstPlayerRegistration, 5},
            SpawnIdentity{NpcSpawnId::SERA_FLINT_SPAWN, NpcType::SERA_FLINT,
                "npc.sera_flint.001", "npc.sera_flint", "spawn.sera_flint",
                PlacementKind::NpcSpawn, InitializationStage::FirstPlayerRegistration, 6}};

        [[noreturn]] void Fail(const std::string &message)
        {
            throw std::runtime_error("starter world export: " + message);
        }

        template <typename Identities, typename Predicate>
        const auto &FindIdentity(const Identities &identities, Predicate predicate)
        {
            const auto found = std::find_if(identities.begin(), identities.end(), predicate);
            if (found == identities.end())
                Fail("compiled content has no explicit identity mapping");
            return *found;
        }

        const ResourceIdentity &Find(ResourceType type)
        {
            return FindIdentity(ResourceIdentities, [type](const auto &entry) { return entry.type == type; });
        }

        const StationIdentity &Find(StationType type)
        {
            return FindIdentity(StationIdentities, [type](const auto &entry) { return entry.type == type; });
        }

        const SpawnIdentity &Find(NpcSpawnId id)
        {
            return FindIdentity(SpawnIdentities, [id](const auto &entry) { return entry.spawnId == id; });
        }

        char TerrainSymbol(TileType type)
        {
            switch (type)
            {
            case TileType::GRASS: return 'G';
            case TileType::DIRT: return 'D';
            case TileType::STONE: return 'S';
            case TileType::WATER: return 'W';
            default: Fail("unsupported base terrain; occupancy is not terrain");
            }
        }

        const char *KindName(PlacementKind kind)
        {
            switch (kind)
            {
            case PlacementKind::Resource: return "resource";
            case PlacementKind::Station: return "station";
            case PlacementKind::NpcSpawn: return "npc_spawn";
            case PlacementKind::MonsterSpawn: return "monster_spawn";
            }
            Fail("unsupported placement kind");
        }

        const char *StageName(InitializationStage stage)
        {
            switch (stage)
            {
            case InitializationStage::WorldStart: return "world_start";
            case InitializationStage::FirstPlayerRegistration: return "first_player_registration";
            }
            Fail("unsupported initialization stage");
        }

        void CheckSize(int width, int height)
        {
            // This writer's limits are deliberately independent of an eventual
            // untrusted-input parser. Validate before calling unchecked Map APIs.
            constexpr int MaximumDimension = 1024;
            if (width <= 0 || height <= 0 ||
                width > MaximumDimension || height > MaximumDimension)
                Fail("dimensions must be in the range 1..1024");
        }

        bool InBounds(const Snapshot &snapshot, const Position &position)
        {
            return position.GetX() >= 0 && position.GetY() >= 0 &&
                position.GetX() < snapshot.width && position.GetY() < snapshot.height;
        }

        void CheckIdentity(const std::string &value)
        {
            if (value.empty() || value.size() > 128 ||
                !std::all_of(value.begin(), value.end(), [](unsigned char ch) {
                    return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
                        ch == '.' || ch == '_' || ch == '-';
                }))
                Fail("identity must contain 1..128 lowercase ASCII identity characters");
        }

        void Validate(const Snapshot &snapshot)
        {
            CheckSize(snapshot.width, snapshot.height);
            if (snapshot.terrainRows.size() != static_cast<std::size_t>(snapshot.height))
                Fail("terrain row count does not match height");
            for (const auto &row : snapshot.terrainRows)
            {
                if (row.size() != static_cast<std::size_t>(snapshot.width) ||
                    row.find_first_not_of("GDSW") != std::string::npos)
                    Fail("terrain rows must have width ASCII symbols from G/D/S/W");
            }
            if (!InBounds(snapshot, snapshot.playerSpawn))
                Fail("player spawn is outside the map");
            if (snapshot.placements.size() > 4096)
                Fail("placement count exceeds 4096");

            std::set<std::string> ids;
            std::set<std::pair<InitializationStage, int>> orderKeys;
            for (const Placement &placement : snapshot.placements)
            {
                CheckIdentity(placement.id);
                CheckIdentity(placement.definitionId);
                if (!ids.insert(placement.id).second)
                    Fail("duplicate placement ID: " + placement.id);
                KindName(placement.kind);
                StageName(placement.stage);
                if (placement.order < 0 || !orderKeys.emplace(placement.stage, placement.order).second)
                    Fail("negative or duplicate initialization order: " + placement.id);
                if (!InBounds(snapshot, placement.position))
                    Fail("placement outside map: " + placement.id);

                if (const auto *resource = std::get_if<ResourceType>(&placement.legacyDefinition))
                {
                    if (placement.kind != PlacementKind::Resource || placement.definitionId != Find(*resource).definitionId)
                        Fail("resource identity/kind mismatch: " + placement.id);
                }
                else if (const auto *station = std::get_if<StationType>(&placement.legacyDefinition))
                {
                    if (placement.kind != PlacementKind::Station || placement.definitionId != Find(*station).definitionId)
                        Fail("station identity/kind mismatch: " + placement.id);
                }
                else
                {
                    const auto &spawn = std::get<NpcSpawnDefinition>(placement.legacyDefinition);
                    const auto &identity = Find(spawn.spawnId);
                    const auto *definition = NpcDefinitionDatabase::TryGet(spawn.npcType);
                    if (spawn.npcType != identity.npcType || definition == nullptr ||
                        placement.kind != identity.kind || placement.definitionId != identity.definitionId ||
                        (definition->kind == NpcKind::MONSTER) != (placement.kind == PlacementKind::MonsterSpawn))
                        Fail("spawn identity/kind mismatch: " + placement.id);
                    if (spawn.spawnPosition.GetX() != placement.position.GetX() ||
                        spawn.spawnPosition.GetY() != placement.position.GetY() ||
                        spawn.wanderRadius < 0 || spawn.wanderIntervalTicks < 0 || spawn.maximumActiveCount <= 0)
                        Fail("invalid spawn configuration: " + placement.id);
                }
            }
        }

        std::string Quote(std::string_view value)
        {
            constexpr char Hex[] = "0123456789abcdef";
            std::string result = "\"";
            for (unsigned char ch : value)
            {
                switch (ch)
                {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (ch < 0x20)
                    {
                        result += "\\u00";
                        result += Hex[ch >> 4];
                        result += Hex[ch & 15];
                    }
                    else
                        result += static_cast<char>(ch);
                }
            }
            return result + '"';
        }

        void WritePosition(std::ostream &out, const Position &position)
        {
            out << "{ \"x\": " << position.GetX() << ", \"y\": " << position.GetY()
                << ", \"surface_id\": \"ground\" }";
        }
    }

    Snapshot Capture()
    {
        Snapshot snapshot;
        snapshot.width = DevelopmentWorldContent::MapWidth;
        snapshot.height = DevelopmentWorldContent::MapHeight;
        snapshot.playerSpawn = Position{DevelopmentWorldContent::PlayerSpawnX, DevelopmentWorldContent::PlayerSpawnY};
        CheckSize(snapshot.width, snapshot.height);
        Map ground(snapshot.width, snapshot.height);
        for (const auto &region : DevelopmentWorldContent::GetStarterTerrainRegions())
        {
            TerrainSymbol(region.tileType);
            if (region.minimumX > region.maximumX || region.minimumY > region.maximumY ||
                !ground.IsInBounds(region.minimumX, region.minimumY) ||
                !ground.IsInBounds(region.maximumX, region.maximumY))
                Fail("compiled terrain region is outside the map");
            for (int y = region.minimumY; y <= region.maximumY; ++y)
                for (int x = region.minimumX; x <= region.maximumX; ++x)
                    if (ground.GetTileType(x, y) != TileType::WATER)
                        ground.SetTileType(x, y, region.tileType);
        }
        for (int y = 0; y < snapshot.height; ++y)
        {
            std::string row;
            row.reserve(static_cast<std::size_t>(snapshot.width));
            for (int x = 0; x < snapshot.width; ++x)
                row += TerrainSymbol(ground.GetTileType(x, y));
            snapshot.terrainRows.push_back(std::move(row));
        }

        std::set<NpcSpawnId> spawnIds;
        for (const auto &spawn : NpcSpawnDatabase::GetStarterSpawns())
        {
            const auto &identity = Find(spawn.spawnId);
            if (!spawnIds.insert(spawn.spawnId).second)
                Fail("duplicate compiled spawn mapping");
            snapshot.placements.push_back({identity.id, identity.definitionId, identity.kind,
                identity.stage, identity.order, spawn.spawnPosition, spawn});
        }
        if (spawnIds.size() != SpawnIdentities.size())
            Fail("compiled spawn registry no longer matches the export bridge");

        int order = 3; // Guide and two initial monsters precede all world objects.
        std::set<ResourceType> resourceTypes;
        for (const auto &resource : DevelopmentWorldContent::GetStarterResourcePlacements())
        {
            const auto &identity = Find(resource.resourceType);
            if (!resourceTypes.insert(resource.resourceType).second)
                Fail("repeated resource type needs an explicit new placement identity");
            snapshot.placements.push_back({identity.id, identity.definitionId, PlacementKind::Resource,
                InitializationStage::WorldStart, order++, Position{resource.x, resource.y}, resource.resourceType});
        }
        if (resourceTypes.size() != ResourceIdentities.size())
            Fail("compiled resource registry no longer matches the export bridge");

        std::set<StationType> stationTypes;
        for (const auto &station : DevelopmentWorldContent::GetStarterStationPlacements())
        {
            const auto &identity = Find(station.stationType);
            if (!stationTypes.insert(station.stationType).second)
                Fail("repeated station type needs an explicit new placement identity");
            snapshot.placements.push_back({identity.id, identity.definitionId, PlacementKind::Station,
                InitializationStage::WorldStart, order++, Position{station.x, station.y}, station.stationType});
        }
        if (stationTypes.size() != StationIdentities.size())
            Fail("compiled station registry no longer matches the export bridge");
        Validate(snapshot);
        return snapshot;
    }

    std::string SerializeCanonical(const Snapshot &snapshot)
    {
        Validate(snapshot);
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << "{\n"
            << "  \"format\": \"embervale.starter-world-snapshot\",\n"
            << "  \"schema_version\": 1,\n"
            << "  \"region_id\": \"embervale.development_starter\",\n"
            << "  \"revision\": 1,\n"
            << "  \"size\": { \"width\": " << snapshot.width << ", \"height\": " << snapshot.height << " },\n"
            << "  \"surfaces\": [ { \"id\": \"ground\", \"elevation\": 0 } ],\n"
            << "  \"terrain\": {\n"
            << "    \"encoding\": \"rows\",\n"
            << "    \"legend\": { \"G\": \"terrain.grass\", \"D\": \"terrain.dirt\", \"S\": \"terrain.stone\", \"W\": \"terrain.water\" },\n"
            << "    \"rows\": [\n";
        for (std::size_t i = 0; i < snapshot.terrainRows.size(); ++i)
            out << "      " << Quote(snapshot.terrainRows[i]) << (i + 1 == snapshot.terrainRows.size() ? "\n" : ",\n");
        out << "    ]\n  },\n  \"player_spawn\": ";
        WritePosition(out, snapshot.playerSpawn);
        out << ",\n  \"placements\": [\n";

        std::vector<const Placement *> sorted;
        sorted.reserve(snapshot.placements.size());
        for (const auto &placement : snapshot.placements)
            sorted.push_back(&placement);
        std::sort(sorted.begin(), sorted.end(), [](const Placement *left, const Placement *right) {
            return left->id < right->id;
        });
        for (std::size_t i = 0; i < sorted.size(); ++i)
        {
            const auto &placement = *sorted[i];
            out << "    {\n"
                << "      \"id\": " << Quote(placement.id) << ",\n"
                << "      \"kind\": " << Quote(KindName(placement.kind)) << ",\n"
                << "      \"definition_id\": " << Quote(placement.definitionId) << ",\n"
                << "      \"initialization\": { \"stage\": " << Quote(StageName(placement.stage))
                << ", \"order\": " << placement.order << " },\n"
                << "      \"position\": ";
            WritePosition(out, placement.position);
            if (const auto *spawn = std::get_if<NpcSpawnDefinition>(&placement.legacyDefinition))
            {
                out << ",\n      \"spawn\": { \"spawn_id\": " << Quote(Find(spawn->spawnId).spawnReference)
                    << ", \"wander_radius\": " << spawn->wanderRadius
                    << ", \"wander_interval_ticks\": " << spawn->wanderIntervalTicks
                    << ", \"maximum_active_count\": " << spawn->maximumActiveCount
                    << ", \"respawns\": " << (spawn->respawns ? "true" : "false") << " }";
            }
            out << "\n    }" << (i + 1 == sorted.size() ? "\n" : ",\n");
        }
        out << "  ],\n  \"region_exits\": []\n}\n";
        return out.str();
    }
}
