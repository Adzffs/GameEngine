#include "TestSupport.h"
#include "../src/Content/Import/StarterWorldRuntimeInput.h"
#include "../src/Content/Export/StarterWorldSnapshotInput.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace
{
    using namespace StarterWorldExport;

    Placement &NormalTree(Snapshot &snapshot)
    {
        for (auto &placement : snapshot.placements)
            if (const auto *type = std::get_if<ResourceType>(&placement.legacyDefinition);
                type && *type == ResourceType::NORMAL_TREE)
                return placement;
        throw std::runtime_error("normal tree fixture missing");
    }

    Placement &Guide(Snapshot &snapshot)
    {
        for (auto &placement : snapshot.placements)
            if (const auto *spawn = std::get_if<NpcSpawnDefinition>(&placement.legacyDefinition);
                spawn && spawn->spawnId == NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN)
                return placement;
        throw std::runtime_error("guide fixture missing");
    }

    void Reject(TestContext &test, const Snapshot &snapshot, const std::string &message)
    {
        bool rejected = false;
        try { (void)StarterWorldImport::ValidateForRuntime(snapshot); }
        catch (const std::runtime_error &error) { rejected = std::string(error.what()).size() > 0; }
        test.Expect(rejected, message);
    }

    struct TemporaryDirectory
    {
        std::filesystem::path path = std::filesystem::temp_directory_path() /
            ("embervale-runtime-input-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        TemporaryDirectory() { std::filesystem::create_directory(path); }
        ~TemporaryDirectory() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
    };

    void Write(const std::filesystem::path &path, const std::string &bytes)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!file) throw std::runtime_error("could not prepare test input");
    }

    void RejectFile(TestContext &test, const std::filesystem::path &path, const std::string &message)
    {
        bool rejected = false;
        try { (void)StarterWorldImport::LoadFile(path); }
        catch (const std::runtime_error &) { rejected = true; }
        test.Expect(rejected, message);
    }
}

int main()
{
    TestContext test;
    const auto baseline = Capture();
    const auto canonical = SerializeCanonical(baseline);
    test.ExpectEqual(SerializeCanonical(StarterWorldImport::ValidateForRuntime(baseline)),
        canonical, "unchanged baseline passes runtime validation without semantic changes");

    auto candidate = baseline;
    std::reverse(candidate.placements.begin(), candidate.placements.end());
    test.ExpectEqual(SerializeCanonical(StarterWorldImport::ValidateForRuntime(candidate)),
        canonical, "array order does not change the runtime validation result");

    candidate = baseline;
    NormalTree(candidate).position = Position{30, 30};
    auto accepted = StarterWorldImport::ValidateForRuntime(candidate);
    test.ExpectEqual(NormalTree(accepted).position.GetX(), 30, "legal move accepted");
    test.Expect(candidate.terrainRows[5][5] == 'G' && candidate.terrainRows[30][30] == 'G',
        "moving an object does not alter either ground cell");

    candidate = baseline;
    candidate.width = -1;
    Reject(test, candidate, "negative dimensions rejected before map allocation");
    candidate = baseline;
    candidate.width = 101;
    Reject(test, candidate, "runtime resizing rejected while parser remains independently bounded");
    candidate = baseline;
    candidate.terrainRows.pop_back();
    Reject(test, candidate, "incomplete rows rejected before indexed terrain access");
    candidate = baseline;
    NormalTree(candidate).position = Position{-1, 5};
    Reject(test, candidate, "out-of-bounds object rejected before occupancy indexing");
    candidate = baseline;
    candidate.playerSpawn = Position{100, 0};
    Reject(test, candidate, "out-of-bounds player spawn rejected before indexing");

    candidate = baseline;
    NormalTree(candidate).position = Position{10, 10};
    Reject(test, candidate, "object on water rejected");
    candidate = baseline;
    NormalTree(candidate).position = Position{8, 5};
    Reject(test, candidate, "overlapping resources rejected");
    candidate = baseline;
    NormalTree(candidate).position = Position{20, 3};
    Reject(test, candidate, "resource overlapping bank rejected");
    candidate = baseline;
    NormalTree(candidate).position = Position{16, 10};
    Reject(test, candidate, "overlap with delayed supporting actor rejected before world creation");

    candidate = baseline;
    candidate.terrainRows[0][0] = 'W';
    Reject(test, candidate, "blocked player spawn rejected");
    candidate = baseline;
    candidate.playerSpawn = Position{5, 5};
    Reject(test, candidate, "player spawn on resource rejected");
    candidate = baseline;
    candidate.playerSpawn = Position{3, 3};
    Reject(test, candidate, "player spawn on actor rejected");

    candidate = baseline;
    NormalTree(candidate).position = Position{30, 30};
    candidate.terrainRows[30][29] = 'W';
    candidate.terrainRows[30][31] = 'W';
    candidate.terrainRows[29][30] = 'W';
    candidate.terrainRows[31][30] = 'W';
    Reject(test, candidate, "resource without reachable approach rejected");
    candidate = baseline;
    auto &guide = Guide(candidate);
    guide.position = Position{30, 30};
    std::get<NpcSpawnDefinition>(guide.legacyDefinition).spawnPosition = guide.position;
    for (auto &row : candidate.terrainRows) row[25] = 'W';
    Reject(test, candidate, "actor across impassable river rejected");

    candidate = baseline;
    auto &tree = NormalTree(candidate);
    tree.legacyDefinition = ResourceType::OAK_TREE;
    tree.definitionId = "resource.oak_tree";
    Reject(test, candidate, "typed model cannot rebind a stable normal-tree identity to oak");
    candidate = baseline;
    auto &forgedGuide = std::get<NpcSpawnDefinition>(Guide(candidate).legacyDefinition);
    forgedGuide.spawnId = NpcSpawnId::TORREN_ASH_SPAWN;
    forgedGuide.npcType = NpcType::TORREN_ASH;
    Reject(test, candidate, "typed model cannot rebind a guide identity to another valid spawn");
    candidate = baseline;
    ++NormalTree(candidate).order;
    Reject(test, candidate, "typed model cannot reorder initialization");
    candidate = baseline;
    candidate.placements.pop_back();
    Reject(test, candidate, "typed model cannot remove a required identity");
    test.ExpectEqual(SerializeCanonical(baseline), canonical, "failed candidates leave baseline unchanged");

    TemporaryDirectory temporary;
    const auto path = temporary.path / "world.json";
    Write(path, canonical);
    test.ExpectEqual(SerializeCanonical(StarterWorldImport::LoadFile(path)), canonical,
        "bounded file reader loads exact baseline");
    RejectFile(test, temporary.path / "missing.json", "missing file fails explicitly");
    RejectFile(test, temporary.path, "directory is rejected instead of treated as content");
    Write(path, "");
    RejectFile(test, path, "empty input fails explicitly");
    Write(path, "{\"format\":");
    RejectFile(test, path, "truncated file fails explicitly");
    Write(path, canonical + std::string(MaximumSnapshotInputBytes - canonical.size(), ' '));
    test.ExpectEqual(SerializeCanonical(StarterWorldImport::LoadFile(path)), canonical,
        "exact byte limit is accepted with legal trailing whitespace");
    Write(path, canonical + std::string(MaximumSnapshotInputBytes - canonical.size() + 1, ' '));
    RejectFile(test, path, "one byte beyond the file limit is rejected");
    return test.Finish();
}
