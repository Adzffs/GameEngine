#include "StarterWorldSnapshotInput.h"

#include <algorithm>
#include <charconv>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace StarterWorldExport
{
    namespace
    {
        struct Json
        {
            enum class Type { Object, Array, String, Number, Boolean, Null } type;
            std::map<std::string, Json> object;
            std::vector<Json> array;
            std::string string;
            int number = 0;
            bool boolean = false;
        };

        class Reader
        {
        public:
            explicit Reader(std::string_view bytes) : bytes_(bytes) {}

            Json Read()
            {
                if (bytes_.size() > MaximumSnapshotInputBytes)
                    Fail("input exceeds 2 MiB");
                Json result = Value(0);
                Space();
                if (cursor_ != bytes_.size())
                    Fail("trailing bytes");
                return result;
            }

        private:
            std::string_view bytes_;
            std::size_t cursor_ = 0;
            std::size_t valueCount_ = 0;

            [[noreturn]] void Fail(const char *message) const
            {
                throw std::runtime_error("starter world input at byte " +
                    std::to_string(cursor_) + ": " + message);
            }

            void Space()
            {
                while (cursor_ < bytes_.size() &&
                    (bytes_[cursor_] == ' ' || bytes_[cursor_] == '\n' ||
                     bytes_[cursor_] == '\r' || bytes_[cursor_] == '\t'))
                    ++cursor_;
            }

            bool Take(char ch)
            {
                Space();
                if (cursor_ < bytes_.size() && bytes_[cursor_] == ch)
                {
                    ++cursor_;
                    return true;
                }
                return false;
            }

            void Require(char ch)
            {
                if (!Take(ch))
                    Fail("unexpected token");
            }

            std::string String()
            {
                Require('"');
                std::string result;
                while (cursor_ < bytes_.size())
                {
                    const unsigned char ch = static_cast<unsigned char>(bytes_[cursor_++]);
                    if (ch == '"')
                        return result;
                    if (ch < 0x20 || ch >= 0x80)
                        Fail("unsupported string character (ASCII only)");
                    if (result.size() >= 16384)
                        Fail("string too long");
                    if (ch != '\\')
                    {
                        result += static_cast<char>(ch);
                        continue;
                    }
                    if (cursor_ >= bytes_.size())
                        Fail("incomplete escape");
                    const char escaped = bytes_[cursor_++];
                    switch (escaped)
                    {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default: Fail("unsupported escape");
                    }
                }
                Fail("unterminated string");
            }

            Json Value(int depth)
            {
                if (depth > 12)
                    Fail("nesting too deep");
                // A byte limit alone permits many tiny values with large DOM
                // overhead, spread across individually legal arrays/objects.
                if (++valueCount_ > 65536)
                    Fail("too many JSON values");
                Space();
                if (cursor_ == bytes_.size())
                    Fail("unexpected end");
                if (Take('{'))
                {
                    Json value{Json::Type::Object};
                    if (Take('}')) return value;
                    do
                    {
                        if (value.object.size() >= 32)
                            Fail("too many object keys");
                        const std::string key = String();
                        Require(':');
                        Json child = Value(depth + 1);
                        if (!value.object.emplace(key, std::move(child)).second)
                            Fail("duplicate object key");
                        if (Take('}')) return value;
                        Require(',');
                    } while (true);
                }
                if (Take('['))
                {
                    Json value{Json::Type::Array};
                    if (Take(']')) return value;
                    do
                    {
                        if (value.array.size() >= 16384)
                            Fail("array too large");
                        value.array.push_back(Value(depth + 1));
                        if (Take(']')) return value;
                        Require(',');
                    } while (true);
                }
                if (bytes_[cursor_] == '"')
                {
                    Json value{Json::Type::String};
                    value.string = String();
                    return value;
                }
                if (bytes_[cursor_] == '-' || (bytes_[cursor_] >= '0' && bytes_[cursor_] <= '9'))
                {
                    const std::size_t start = cursor_;
                    if (bytes_[cursor_] == '-') ++cursor_;
                    if (cursor_ == bytes_.size()) Fail("invalid integer");
                    if (bytes_[cursor_] == '0') ++cursor_;
                    else
                    {
                        if (bytes_[cursor_] < '1' || bytes_[cursor_] > '9')
                            Fail("invalid integer");
                        while (cursor_ < bytes_.size() && bytes_[cursor_] >= '0' && bytes_[cursor_] <= '9')
                            ++cursor_;
                    }
                    int parsed = 0;
                    const auto conversion = std::from_chars(bytes_.data() + start, bytes_.data() + cursor_, parsed);
                    if (conversion.ec != std::errc{} || conversion.ptr != bytes_.data() + cursor_)
                        Fail("integer overflow");
                    Json value{Json::Type::Number};
                    value.number = parsed;
                    return value;
                }
                for (const auto keyword : {"true", "false", "null"})
                {
                    const std::string_view word(keyword);
                    if (bytes_.substr(cursor_, word.size()) == word)
                    {
                        cursor_ += word.size();
                        Json value{word == "null" ? Json::Type::Null : Json::Type::Boolean};
                        value.boolean = word == "true";
                        return value;
                    }
                }
                Fail("invalid JSON value");
            }
        };

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
        const Json root = Reader(input).Read();
        Keys(root, {"format", "schema_version", "region_id", "revision", "size",
            "surfaces", "terrain", "player_spawn", "placements", "region_exits"});
        Expect(String(root, "format"), "embervale.starter-world-snapshot");
        if (Number(root, "schema_version") != 1 || Number(root, "revision") != 1)
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

        const Json &exits = Field(root, "region_exits");
        if (exits.type != Json::Type::Array || !exits.array.empty())
            Invalid("region exits are not supported");

        const Json &placements = Field(root, "placements");
        if (placements.type != Json::Type::Array || placements.array.size() > 4096)
            Invalid("invalid placement array");

        // The current format is an export snapshot, not an open-ended authored
        // catalogue. Bind each input identity to the reviewed compiled registry.
        // Positions may change, but unknown IDs/definitions and lifecycle edits
        // must wait for the dedicated authored-model migration.
        const Snapshot baseline = Capture();
        std::map<std::string, Placement> known;
        for (const auto &placement : baseline.placements)
            known.emplace(placement.id, placement);

        for (const Json &item : placements.array)
        {
            if (item.type != Json::Type::Object)
                Invalid("placement must be an object");
            const std::string &id = String(item, "id");
            const auto found = known.find(id);
            if (found == known.end())
                Invalid("unknown or duplicate placement ID: " + id);
            Placement placement = found->second;
            known.erase(found);
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
                Keys(spawnJson, {"spawn_id", "wander_radius", "wander_interval_ticks",
                    "maximum_active_count", "respawns"});
                auto &spawn = std::get<NpcSpawnDefinition>(placement.legacyDefinition);
                // Capture's explicit spawn registry binds each stable placement ID.
                // Match the reviewed baseline spawn token by reading the canonical
                // baseline representation rather than trusting a supplied enum.
                if (Number(spawnJson, "wander_radius") != spawn.wanderRadius ||
                    Number(spawnJson, "wander_interval_ticks") != spawn.wanderIntervalTicks ||
                    Number(spawnJson, "maximum_active_count") != spawn.maximumActiveCount ||
                    Boolean(spawnJson, "respawns") != spawn.respawns)
                    Invalid("unsupported spawn configuration change: " + id);
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
        if (!known.empty())
            Invalid("missing compiled placement identities");

        // Reuse the writer's structural and typed validation, then compare
        // the caller's spawn references with canonical registry bindings.
        const std::string canonical = SerializeCanonical(snapshot);
        const Json checked = Reader(canonical).Read();
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
