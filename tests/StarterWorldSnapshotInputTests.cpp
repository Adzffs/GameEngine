#include "../src/Content/Export/StarterWorldSnapshotInput.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace StarterWorldExport;

namespace
{
    bool Rejects(const std::string &input, std::string_view diagnostic = {})
    {
        try { (void)ParseSnapshot(input); }
        catch (const std::runtime_error &error)
        {
            return std::string_view(error.what()).find(diagnostic) != std::string_view::npos;
        }
        return false;
    }

    std::string ReplaceOnce(std::string input, const std::string &from,
        const std::string &to)
    {
        const auto pos = input.find(from);
        assert(pos != std::string::npos);
        input.replace(pos, from.size(), to);
        return input;
    }

    Placement &FindPlacement(Snapshot &snapshot, const std::string &id)
    {
        const auto found = std::find_if(snapshot.placements.begin(), snapshot.placements.end(),
            [&id](const Placement &placement) { return placement.id == id; });
        assert(found != snapshot.placements.end());
        return *found;
    }

    void CheckRoundTripAndEdits(const std::string &canonical)
    {
        assert(SerializeCanonical(ParseSnapshot(canonical)) == canonical);
        assert(SerializeCanonical(ParseSnapshot(" \n" + canonical + "\t")) == canonical);
        assert(SerializeCanonical(ParseSnapshot(ReplaceOnce(canonical,
            "\"x\": 0, \"y\": 0", "\"y\": 0, \"x\": 0"))) == canonical);

        // Change only supported content and verify typed spawn coordinates travel
        // with their stable identity rather than remaining at the compiled point.
        Snapshot changed = Capture();
        changed.terrainRows[30][30] = 'D';
        changed.playerSpawn.SetPosition(2, 0);
        FindPlacement(changed, "resource.normal_tree.001").position.SetPosition(6, 5);
        auto &monster = FindPlacement(changed, "monster.passive_development.001");
        monster.position.SetPosition(7, 1);
        std::get<NpcSpawnDefinition>(monster.legacyDefinition).spawnPosition = monster.position;
        const std::string edited = SerializeCanonical(changed);
        Snapshot parsed = ParseSnapshot(edited);
        assert(SerializeCanonical(parsed) == edited);
        const auto &parsedMonster = FindPlacement(parsed, "monster.passive_development.001");
        assert(parsedMonster.order == monster.order);
        assert(parsedMonster.stage == monster.stage);
        assert(std::get<NpcSpawnDefinition>(parsedMonster.legacyDefinition).spawnPosition.GetX() == 7);
        assert(SerializeCanonical(Capture()) == canonical);

        // JSON array order is independent of authored initialization order.
        const auto begin = canonical.find("  \"placements\": [\n") +
            std::string("  \"placements\": [\n").size();
        const auto firstEnd = canonical.find("\n    },\n", begin) +
            std::string("\n    }").size();
        const auto arrayEnd = canonical.find("\n  ],\n  \"region_exits\"", firstEnd);
        assert(arrayEnd != std::string::npos);
        const std::string first = canonical.substr(begin, firstEnd - begin);
        const std::string reordered = canonical.substr(0, begin) +
            canonical.substr(firstEnd + 2, arrayEnd - firstEnd - 2) +
            ",\n" + first + canonical.substr(arrayEnd);
        assert(SerializeCanonical(ParseSnapshot(reordered)) == canonical);
        const std::string duplicate = canonical.substr(0, arrayEnd) +
            ",\n" + first + canonical.substr(arrayEnd);
        assert(Rejects(duplicate, "duplicate placement ID"));
    }

    void CheckMalformedJson(const std::string &canonical)
    {
        for (const std::string input : {"", "{}", "null", "[]", "{", "[1,]", "{\"x\":1,}",
            "{\"x\":01}", "{\"x\":-}", "{\"x\":1.5}", "{\"x\":1e3}",
            "{\"x\":2147483648}", "{\"x\":-2147483649}", "{\"x\":NaN}",
            "{\"x\":truefalse}", "{\"x\":\"unterminated}", "{\"x\":\"\\u0061\"}",
            "{\"x\":\"\\q\"}", "{\"x\":\"\n\"}"})
            assert(Rejects(input));
        assert(Rejects(canonical + " garbage"));
        assert(Rejects(canonical + '\0'));
        assert(Rejects(std::string("\xEF\xBB\xBF") + canonical));
        assert(Rejects("{\"x\":\"" + std::string(1, static_cast<char>(0x80)) + "\"}"));
        assert(Rejects(ReplaceOnce(canonical, "\"format\":", "\"format\": null, \"format\":"),
            "duplicate object key"));
        assert(Rejects(ReplaceOnce(canonical, "\"width\": 100", "\"width\": 100, \"width\": 100"),
            "duplicate object key"));
    }

    void CheckSchemaAndIdentity(const std::string &canonical)
    {
        for (const auto &replacement : {
            std::pair{"\"schema_version\": 1", "\"schema_version\": 2"},
            std::pair{"\"revision\": 1", "\"revision\": 2"},
            std::pair{"\"region_exits\": []", "\"region_exits\": [1]"},
            std::pair{"\"region_exits\": []", "\"region_exits\": [], \"unknown\": 0"},
            std::pair{"\"width\": 100", "\"width\": 0"},
            std::pair{"\"width\": 100", "\"width\": -1"},
            std::pair{"\"width\": 100", "\"width\": 1025"},
            std::pair{"\"width\": 100", "\"width\": 99"},
            std::pair{"\"width\": 100", "\"width\": true"},
            std::pair{"\"height\": 100", "\"height\": 99"},
            std::pair{"\"elevation\": 0", "\"elevation\": 1"},
            std::pair{"\"G\": \"terrain.grass\"", "\"G\": \"terrain.water\""},
            std::pair{"\"surface_id\": \"ground\"", "\"surface_id\": \"upper\""},
            std::pair{"\"x\": 0", "\"x\": -1"},
            std::pair{"\"x\": 0", "\"x\": 100"},
            std::pair{"\"x\": 18", "\"x\": 2147483647"},
            std::pair{"\"id\": \"resource.normal_tree.001\"", "\"id\": \"resource.unknown.001\""},
            std::pair{"\"definition_id\": \"resource.normal_tree\"", "\"definition_id\": \"resource.oak_tree\""},
            std::pair{"\"kind\": \"resource\"", "\"kind\": \"station\""},
            std::pair{"\"stage\": \"world_start\"", "\"stage\": \"first_player_registration\""},
            std::pair{"\"order\": 2", "\"order\": 99"},
            std::pair{"\"spawn_id\": \"spawn.aggressive_development\"", "\"spawn_id\": \"spawn.passive_development\""},
            std::pair{"\"wander_radius\": 0", "\"wander_radius\": 1"},
            std::pair{"\"wander_interval_ticks\": 0", "\"wander_interval_ticks\": 1"},
            std::pair{"\"maximum_active_count\": 1", "\"maximum_active_count\": 2"},
            std::pair{"\"respawns\": true", "\"respawns\": false"},
            std::pair{"\"respawns\": true", "\"respawns\": 1"}})
            assert(Rejects(ReplaceOnce(canonical, replacement.first, replacement.second)));

        const auto firstRow = canonical.find("\"rows\": [\n      \"") +
            std::string("\"rows\": [\n      \"").size();
        std::string invalidTerrain = canonical;
        invalidTerrain[firstRow] = 'X';
        assert(Rejects(invalidTerrain, "invalid terrain row"));
        invalidTerrain[firstRow] = 'g';
        assert(Rejects(invalidTerrain, "invalid terrain row"));

        Snapshot missing = Capture();
        missing.placements.pop_back();
        assert(Rejects(SerializeCanonical(missing), "missing compiled placement identities"));
        missing.placements.clear();
        assert(Rejects(SerializeCanonical(missing), "missing compiled placement identities"));
    }

    void CheckResourceBounds(const std::string &canonical)
    {
        // Diagnostic checks distinguish early budget rejection from a later
        // schema error, so these exercise actual allocation bounds.
        assert(Rejects(std::string(MaximumSnapshotInputBytes + 1, ' '), "input exceeds"));
        std::string padded = canonical;
        padded.resize(MaximumSnapshotInputBytes, ' ');
        assert(SerializeCanonical(ParseSnapshot(padded)) == canonical);
        assert(Rejects(std::string(32, '[') + "0" + std::string(32, ']'), "nesting too deep"));
        assert(Rejects("\"" + std::string(16385, 'a') + "\"", "string too long"));
        assert(Rejects("\"" + std::string(16384, 'a') + "\\n\"", "string too long"));
        assert(Rejects("\"" + std::string(16384, 'a') + "\"", "unexpected object fields"));

        std::string array = "[0";
        for (int i = 1; i < 16384; ++i)
            array += ",0";
        array += ']';
        assert(Rejects(array, "unexpected object fields"));
        assert(Rejects(array.substr(0, array.size() - 1) + ",0]", "array too large"));
        assert(Rejects("[" + array + "," + array + "," + array + "," + array + "]",
            "too many JSON values"));

        std::string object = "{";
        for (int i = 0; i < 33; ++i)
            object += (i == 0 ? "" : ",") + std::string("\"") + std::to_string(i) + "\":0";
        object += '}';
        assert(Rejects(object, "too many object keys"));
    }
}

int main()
{
    const std::string canonical = SerializeCanonical(Capture());
    const Snapshot held = ParseSnapshot(canonical);
    CheckRoundTripAndEdits(canonical);
    CheckMalformedJson(canonical);
    CheckSchemaAndIdentity(canonical);
    CheckResourceBounds(canonical);

    // Rejections cannot alter an existing model or the compiled source.
    assert(SerializeCanonical(held) == canonical);
    assert(SerializeCanonical(Capture()) == canonical);
}
