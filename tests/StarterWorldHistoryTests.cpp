#include "TestSupport.h"
#include "../src/Content/Edit/StarterWorldHistory.h"
#include "../src/Content/Edit/StarterWorldEdit.h"
#include "../src/Content/Import/StarterWorldRuntimeInput.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    using namespace StarterWorldExport;
    constexpr const char *Tree = "resource.normal_tree.001";
    constexpr const char *Furnace = "station.furnace.001";

    template <typename Action>
    void Reject(TestContext &test, Action action, const std::string &message)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::runtime_error &) { rejected = true; }
        test.Expect(rejected, message);
    }
}

int main()
{
    TestContext test;
    const Snapshot baseline = Capture();
    const std::string original = SerializeCanonical(baseline);
    StarterWorldHistory::History history(baseline);
    test.Expect(!history.CanUndo() && !history.CanRedo(), "new session has no history");
    Reject(test, [&] { (void)history.Undo(baseline); }, "empty undo rejected");
    Reject(test, [&] { (void)history.Redo(baseline); }, "empty redo rejected");

    const auto edited = history.ApplyMoves(baseline, {
        {Tree, Position{30, 30}}, {Furnace, Position{32, 30}}});
    const std::string expected = SerializeCanonical(edited);
    test.Expect(history.CanUndo() && !history.CanRedo(), "batch is one undoable entry");
    test.ExpectEqual(history.EntryCount(), std::size_t{1}, "two moves produce one entry");
    test.ExpectEqual(history.AppliedCount(), std::size_t{1}, "one batch applied");
    test.Expect(expected != original, "edit changes world semantics");
    test.ExpectEqual(SerializeCanonical(StarterWorldImport::ValidateForRuntime(edited)),
        expected, "batch is runtime valid");
    test.ExpectEqual(SerializeCanonical(baseline), original, "source remains unchanged");

    const auto restored = history.Undo(edited);
    test.ExpectEqual(SerializeCanonical(restored), original, "undo restores exact semantics");
    test.Expect(!history.CanUndo() && history.CanRedo(), "undo exposes redo");
    const auto replayed = history.Redo(restored);
    test.ExpectEqual(SerializeCanonical(replayed), expected, "redo reproduces exact batch");

    const auto external = StarterWorldEdit::MovePlacement(edited, Tree, Position{33, 30});
    Reject(test, [&] { (void)history.Undo(external.candidate); },
        "undo rejects externally modified but valid content");
    test.ExpectEqual(history.AppliedCount(), std::size_t{1},
        "conflict does not advance history cursor");

    Reject(test, [&] { (void)history.ApplyMoves(edited, {
        {Tree, Position{31, 30}}, {Furnace, Position{10, 10}}}); },
        "invalid second move rejects whole batch");
    test.ExpectEqual(history.EntryCount(), std::size_t{1},
        "failed batch does not create an entry");
    test.ExpectEqual(SerializeCanonical(edited), expected,
        "failed batch cannot mutate caller content");

    const auto again = history.Undo(edited);
    Position treeStart{0, 0};
    for (const Placement &placement : baseline.placements)
        if (placement.id == Tree) treeStart = placement.position;
    const auto same = history.ApplyMoves(again, {{Tree, treeStart}});
    test.ExpectEqual(SerializeCanonical(same), original, "no-op preserves content");
    test.Expect(history.CanRedo() && history.EntryCount() == 1,
        "no-op does not discard pending redo");

    const auto branched = history.ApplyMoves(same, {{Furnace, Position{34, 30}}});
    test.Expect(!history.CanRedo(), "new move after undo discards old redo branch");
    test.Expect(history.CanUndo() && history.EntryCount() == 1,
        "replacement branch contains only committed entry");
    Reject(test, [&] { (void)history.Redo(branched); },
        "redo with no pending entry is rejected");
    test.ExpectEqual(SerializeCanonical(history.Undo(branched)), original,
        "replacement branch reverses completely");

    Reject(test, [&] { (void)history.ApplyMoves(baseline, {}); },
        "empty batch is rejected");
    std::vector<StarterWorldHistory::MoveCommand> huge(
        StarterWorldHistory::History::MaximumMovesPerEntry + 1,
        StarterWorldHistory::MoveCommand{Tree, Position{30, 30}});
    Reject(test, [&] { (void)history.ApplyMoves(baseline, huge); },
        "oversized command batch is rejected before applying");
    test.ExpectEqual(SerializeCanonical(baseline), original,
        "original input remains untouched after all operations");

    // Repeated commands against one durable ID are undone in reverse order.
    StarterWorldHistory::History repeated(baseline);
    const auto doubleMove = repeated.ApplyMoves(baseline, {
        {Tree, Position{30, 30}}, {Tree, Position{31, 30}}});
    const auto doubleCanonical = SerializeCanonical(doubleMove);
    const auto doubleUndone = repeated.Undo(doubleMove);
    test.ExpectEqual(SerializeCanonical(doubleUndone), original,
        "undo reverses multiple moves of the same durable ID");
    test.ExpectEqual(SerializeCanonical(repeated.Redo(doubleUndone)), doubleCanonical,
        "redo replays multiple moves of the same durable ID");

    const auto externallyChanged = StarterWorldEdit::MovePlacement(
        doubleUndone, Furnace, Position{35, 30}).candidate;
    repeated.Undo(doubleMove);
    Reject(test, [&] { (void)repeated.Redo(externallyChanged); },
        "redo also rejects a valid external semantic revision");
    test.Expect(repeated.CanRedo(), "redo conflict preserves history cursor");
    return test.Finish();
}
