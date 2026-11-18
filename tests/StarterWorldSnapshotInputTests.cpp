#include "../src/Content/Export/StarterWorldSnapshotInput.h"

#include <cassert>
#include <stdexcept>
#include <string>

using namespace StarterWorldExport;

namespace
{
    bool Rejects(const std::string &input)
    {
        try { (void)ParseSnapshot(input); }
        catch (const std::runtime_error &) { return true; }
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
}

int main()
{
    const std::string canonical = SerializeCanonical(Capture());
    const Snapshot parsed = ParseSnapshot(canonical);
    assert(SerializeCanonical(parsed) == canonical);

    // Input ordering and whitespace are irrelevant to canonical output.
    assert(SerializeCanonical(ParseSnapshot(" \n" + canonical + "\t")) == canonical);

    // A bounded coordinate edit preserves stable identity and lifecycle.
    const std::string moved = ReplaceOnce(canonical,
        "\"id\": \"resource.normal_tree.001\",\n      \"kind\"",
        "\"id\": \"resource.normal_tree.001\",\n      \"kind\"");
    assert(SerializeCanonical(ParseSnapshot(moved)) == canonical);

    assert(Rejects(""));
    assert(Rejects("{}"));
    assert(Rejects(canonical + " garbage"));
    assert(Rejects(ReplaceOnce(canonical, "\"schema_version\": 1", "\"schema_version\": 2")));
    assert(Rejects(ReplaceOnce(canonical, "\"revision\": 1", "\"revision\": 2")));
    assert(Rejects(ReplaceOnce(canonical, "\"region_exits\": []", "\"region_exits\": [1]")));
    assert(Rejects(ReplaceOnce(canonical, "\"format\":", "\"format\": null, \"format\":")));
    assert(Rejects(ReplaceOnce(canonical, "\"width\": 100", "\"width\": 99999")));
    assert(Rejects(ReplaceOnce(canonical, "\"surface_id\": \"ground\"", "\"surface_id\": \"upper\"")));
    assert(Rejects(ReplaceOnce(canonical, "\"id\": \"resource.normal_tree.001\"",
        "\"id\": \"resource.unknown.001\"")));
    assert(Rejects(ReplaceOnce(canonical, "\"definition_id\": \"resource.normal_tree\"",
        "\"definition_id\": \"resource.unknown\"")));
    assert(Rejects(ReplaceOnce(canonical, "\"stage\": \"world_start\"",
        "\"stage\": \"first_player_registration\"")));
    assert(Rejects(ReplaceOnce(canonical, "\"spawn_id\": \"spawn.",
        "\"spawn_id\": \"wrong."));
    assert(Rejects(std::string(2 * 1024 * 1024 + 1, ' ')));
    assert(Rejects(std::string(32, '[') + "0" + std::string(32, ']')));
}
