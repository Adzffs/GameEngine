#include "TestSupport.h"
#include "../src/Content/Edit/StarterWorldEdit.h"
#include "../src/Content/Export/StarterWorldSnapshotInput.h"
#include "../src/Content/Import/StarterWorldRuntimeInput.h"

#include <stdexcept>
#include <string>

namespace
{
    using namespace StarterWorldExport;
    constexpr std::string_view TreeId = "resource.normal_tree.001";
    constexpr std::string_view StationId = "station.furnace.001";

    const Placement &Get(const Snapshot &snapshot, std::string_view id)
    {
        for (const Placement &placement : snapshot.placements)
            if (placement.id == id) return placement;
        throw std::runtime_error("test fixture missing placement");
    }

    void Reject(TestContext &test, const Snapshot &source,
        std::string_view id, const Position &target, const std::string &message)
    {
        bool rejected = false;
        try { (void)StarterWorldEdit::MovePlacement(source, id, target); }
        catch (const std::runtime_error &) { rejected = true; }
        test.Expect(rejected, message);
    }
}

int main()
{
    TestContext test;
    const Snapshot baseline = Capture();
    const std::string original = SerializeCanonical(baseline);
    const Position from = Get(baseline, TreeId).position;

    const auto moved = StarterWorldEdit::MovePlacement(baseline, TreeId, Position{30, 30});
    test.Expect(moved.changed, "moving the tree is reported as a change");
    test.ExpectEqual(moved.placementId, std::string(TreeId), "stable identity is returned");
    test.ExpectEqual(moved.previousPosition.GetX(), from.GetX(), "old x is reported");
    test.ExpectEqual(moved.previousPosition.GetY(), from.GetY(), "old y is reported");
    test.ExpectEqual(Get(moved.candidate, TreeId).position.GetX(), 30, "moved x is authoritative");
    test.ExpectEqual(Get(moved.candidate, TreeId).position.GetY(), 30, "moved y is authoritative");
    test.ExpectEqual(Get(moved.candidate, TreeId).definitionId,
        Get(baseline, TreeId).definitionId, "resource type is preserved");
    test.ExpectEqual(moved.candidate.terrainRows[from.GetY()][from.GetX()],
        baseline.terrainRows[from.GetY()][from.GetX()], "old ground is preserved");
    test.ExpectEqual(moved.candidate.terrainRows[30][30], baseline.terrainRows[30][30],
        "destination ground is preserved");
    test.ExpectEqual(moved.candidate.placements.size(), baseline.placements.size(),
        "no placements added or removed");
    test.ExpectEqual(SerializeCanonical(StarterWorldImport::ValidateForRuntime(moved.candidate)),
        SerializeCanonical(moved.candidate), "edited candidate remains runtime-valid");
    test.ExpectEqual(SerializeCanonical(ParseSnapshot(SerializeCanonical(moved.candidate))),
        SerializeCanonical(moved.candidate), "edited candidate round trips as authored JSON");
    test.ExpectEqual(SerializeCanonical(baseline), original, "source remains unchanged after move");

    const auto same = StarterWorldEdit::MovePlacement(baseline, TreeId, from);
    test.Expect(!same.changed, "same-location move is an explicit no-op");
    test.ExpectEqual(SerializeCanonical(same.candidate), original,
        "no-op keeps the complete canonical snapshot");

    const auto returned = StarterWorldEdit::MovePlacement(moved.candidate, TreeId, from);
    test.Expect(returned.changed, "inverse tree move is accepted");
    test.ExpectEqual(SerializeCanonical(returned.candidate), original,
        "inverse move restores exact original canonical document");

    const auto furnace = StarterWorldEdit::MovePlacement(baseline, StationId, Position{32, 30});
    test.ExpectEqual(Get(furnace.candidate, StationId).position.GetX(), 32,
        "existing station can be relocated with the same operation");
    test.ExpectEqual(SerializeCanonical(baseline), original,
        "station move leaves original untouched");

    Reject(test, baseline, "", Position{30, 30}, "empty ID is rejected");
    Reject(test, baseline, "resource.normal_tree.missing", Position{30, 30},
        "unknown durable ID is rejected");
    Reject(test, baseline, "npc.development_guide.001", Position{30, 30},
        "unsupported actor spawn changes are explicit errors");
    Reject(test, baseline, TreeId, Position{-1, 30}, "out of bounds move rejected");
    Reject(test, baseline, TreeId, Position{100, 30}, "positive out of bounds move rejected");
    Reject(test, baseline, TreeId, Position{10, 10}, "water move rejected");
    Reject(test, baseline, TreeId, Position{8, 5}, "overlapping placement move rejected");
    Reject(test, baseline, TreeId, baseline.playerSpawn, "player spawn collision rejected");

    Snapshot corrupted = baseline;
    corrupted.terrainRows[0][0] = 'W';
    Reject(test, corrupted, TreeId, Position{30, 30},
        "invalid source world is rejected before applying a legal move");
    test.ExpectEqual(SerializeCanonical(baseline), original,
        "failed moves never mutate caller content");
    return test.Finish();
}
