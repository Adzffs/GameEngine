#include "TestSupport.h"
#include "../src/Content/Edit/StarterWorldEdit.h"
#include "../src/Content/Edit/StarterWorldHistory.h"
#include "../src/Content/Export/StarterWorldSnapshotInput.h"
#include "../src/Content/Import/StarterWorldRuntimeInput.h"
#include "../src/Command/ServerCommand.h"
#include "../src/Player/Player.h"
#include "../src/World/World.h"

#include <algorithm>
#include <stdexcept>
#include <string>

using namespace StarterWorldExport;

namespace
{
    template <typename Operation>
    void Reject(TestContext &test, Operation operation, const std::string &message)
    {
        bool rejected = false;
        try { operation(); }
        catch (const std::runtime_error &) { rejected = true; }
        test.Expect(rejected, message);
    }

    bool Has(const Snapshot &world, const std::string &id)
    {
        return std::any_of(world.placements.begin(), world.placements.end(),
            [&id](const Placement &placement) { return placement.id == id; });
    }

    void CheckSchemaAndEdits(TestContext &test)
    {
        const auto baseline = Capture();
        const auto original = SerializeCanonical(baseline);
        auto placed = StarterWorldEdit::PlaceDefinition(baseline, "resource.normal_tree", {40, 20});
        test.ExpectEqual(placed.placement.id, std::string("resource.authored.000001"), "First authored ID is deterministic");
        test.Expect(placed.candidate.schemaVersion == 2 && placed.candidate.nextPlacementId == 2,
            "First object creation upgrades only the candidate to v2 and reserves its ID");
        test.ExpectEqual(SerializeCanonical(ParseSnapshot(SerializeCanonical(placed.candidate))),
            SerializeCanonical(placed.candidate), "Created object and allocation counter round-trip exactly");
        test.ExpectEqual(SerializeCanonical(baseline), original, "Creation leaves its source unchanged");

        const char *definitions[] = {"resource.normal_tree", "resource.oak_tree", "resource.willow_tree",
            "resource.copper_rock", "resource.tin_rock", "resource.iron_rock", "resource.coal_rock",
            "station.furnace", "station.crafting_table", "station.bank"};
        for (const char *definition : definitions)
        {
            const auto candidate = StarterWorldEdit::PlaceDefinition(baseline, definition, {40, 20}).candidate;
            test.Expect(StarterWorldImport::ValidateForRuntime(candidate).placements.size() == 21,
                std::string("Supported definition creates a runtime-valid object: ") + definition);
        }
        for (const auto tile : {Position{0, 0}, Position{10, 10}, Position{5, 5}, Position{-1, 4}, Position{100, 4}})
            Reject(test, [&] { StarterWorldEdit::PlaceDefinition(baseline, "resource.normal_tree", tile); },
                "Spawn, water, occupied and out-of-bounds placements are rejected");
        Reject(test, [&] { StarterWorldEdit::PlaceDefinition(baseline, "npc.development_guide", {40, 20}); },
            "Actor definitions cannot be placed as objects");
        Reject(test, [&] { StarterWorldEdit::RemovePlacement(baseline, "npc.development_guide.001"); },
            "Required actor spawn identities cannot be removed");
        Reject(test, [&] { StarterWorldEdit::RemovePlacement(baseline, "resource.missing.001"); },
            "Missing placement removal is rejected");

        auto removed = StarterWorldEdit::RemovePlacement(placed.candidate, placed.placement.id).candidate;
        removed = ParseSnapshot(SerializeCanonical(removed));
        const auto replacement = StarterWorldEdit::PlaceDefinition(removed, "station.bank", {40, 20});
        test.ExpectEqual(replacement.placement.id, std::string("station.authored.000002"),
            "Delete/save/reopen never recycles an allocated ID, including across kinds");

        auto invalid = placed.candidate;
        invalid.nextPlacementId = 1;
        Reject(test, [&] { ParseSnapshot(SerializeCanonical(invalid)); }, "Unreserved IDs are rejected");
        invalid = placed.candidate;
        invalid.placements.back().order++;
        Reject(test, [&] { ParseSnapshot(SerializeCanonical(invalid)); }, "Authored lifecycle order cannot be forged");
        invalid = placed.candidate;
        invalid.placements.back().id = "resource.authored.000000";
        Reject(test, [&] { ParseSnapshot(SerializeCanonical(invalid)); }, "Zero allocation is invalid");
        invalid = placed.candidate;
        invalid.schemaVersion = 1;
        invalid.nextPlacementId = 1;
        Reject(test, [&] { ParseSnapshot(SerializeCanonical(invalid)); }, "v1 still rejects authored identities");
        invalid = placed.candidate;
        std::erase_if(invalid.placements, [](const Placement &p) { return p.kind == PlacementKind::NpcSpawn; });
        Reject(test, [&] { ParseSnapshot(SerializeCanonical(invalid)); }, "v2 still requires all compiled actor identities");
        invalid = removed;
        invalid.nextPlacementId = MaximumNextPlacementId;
        Reject(test, [&] { StarterWorldEdit::PlaceDefinition(invalid, "station.bank", {40, 20}); },
            "Allocator exhaustion fails without wraparound");

        auto corridor = baseline;
        for (auto &row : corridor.terrainRows) row[25] = 'W';
        corridor.terrainRows[20][25] = 'G';
        corridor = StarterWorldEdit::PlaceDefinition(corridor, "station.bank", {30, 20}).candidate;
        Reject(test, [&] { StarterWorldEdit::PlaceDefinition(corridor, "resource.normal_tree", {25, 20}); },
            "A placement cannot sever the only approach route to another object");
    }

    void CheckHistory(TestContext &test)
    {
        const auto baseline = Capture();
        StarterWorldHistory::History history(baseline);
        auto world = history.ApplyPlace(baseline, "resource.normal_tree", {40, 20});
        const std::string first = "resource.authored.000001";
        world = history.ApplyMoves(world, {{first, {41, 20}}});
        world = history.ApplyPaint(world, {40, 20}, 'S');
        world = history.ApplyRemove(world, first);
        test.Expect(!Has(world, first), "Removal enters the same history as movement and paint");
        world = history.Undo(world);
        test.Expect(Has(world, first) && world.terrainRows[20][40] == 'S', "Undo restores exactly the removed object");
        world = history.Redo(world);
        test.Expect(!Has(world, first), "Redo removes that same object identity");
        world = history.Undo(world);
        world = history.Undo(world);
        world = history.Undo(world);
        world = history.Undo(world);
        test.Expect(!Has(world, first) && world.nextPlacementId == 2,
            "Undo creation restores the map but retains the ID reservation");
        world = history.ApplyPlace(world, "resource.oak_tree", {40, 20});
        test.Expect(Has(world, "resource.authored.000002") && !history.CanRedo(),
            "New branch allocates a new ID instead of recycling the undone object");
        auto foreign = world;
        foreign.nextPlacementId++;
        Reject(test, [&] { history.Undo(foreign); }, "External allocation-counter changes conflict with history");
        const auto before = SerializeCanonical(world);
        const auto cursor = history.AppliedCount();
        Reject(test, [&] { history.ApplyPlace(world, "resource.normal_tree", {0, 0}); },
            "Invalid placement cannot commit history or consume an ID");
        test.Expect(history.AppliedCount() == cursor && SerializeCanonical(world) == before,
            "Rejected placement leaves history and candidate unchanged");
        world = history.ApplyRemove(world, "resource.normal_tree.001");
        world = history.Undo(world);
        test.Expect(Has(world, "resource.normal_tree.001"), "Undo removal preserves a legacy object's original binding");
    }

    void CheckLiveRuntime(TestContext &test)
    {
        auto content = StarterWorldEdit::PlaceDefinition(Capture(), "resource.normal_tree", {40, 20}).candidate;
        content = StarterWorldEdit::PlaceDefinition(content, "station.crafting_table", {42, 20}).candidate;
        content = StarterWorldEdit::PaintTerrain(content, {5, 5}, 'S').candidate;
        content = StarterWorldEdit::RemovePlacement(content, "resource.normal_tree.001").candidate;
        World world(ParseSnapshot(SerializeCanonical(content)), 31U, 37U, 41U);
        test.Expect(!world.GetResourceAt(5, 5) && world.GetMap().GetTileType(5, 5) == TileType::STONE &&
            world.GetMap().IsValidPosition(5, 5), "Removed resource restores its authored ground in the actual game world");
        auto *tree = world.GetResourceAt(40, 20);
        const auto *station = world.GetStationAt(42, 20);
        test.Expect(tree && tree->GetResourceType() == ResourceType::NORMAL_TREE &&
            !world.GetMap().IsValidPosition(40, 20), "New tree is created and blocks movement");
        test.Expect(station && station->GetStationType() == StationType::CRAFTING_TABLE,
            "New crafting table is created through the runtime station factory");
        const int playerId = world.CreatePlayer();
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(playerId));
        if (!player || !tree) throw std::runtime_error("Missing gathering fixture");
        player->GetPosition().SetPosition(39, 20);
        int axeSlot = -1;
        for (int slot = 0; slot < Inventory::SlotCount; ++slot)
            if (player->GetInventory().GetSlots()[slot].GetItemType() == ItemType::BRONZE_AXE) axeSlot = slot;
        world.EnqueueCommand(UseInventoryItemCommand{playerId, axeSlot});
        world.Update();
        world.QueueResourceInteraction(playerId, tree->GetID());
        for (int tick = 0; tick < 64 && player->GetInventory().GetItemAmount(ItemType::LOG) == 0; ++tick)
            world.Update();
        test.Expect(player->GetInventory().GetItemAmount(ItemType::LOG) > 0,
            "A newly authored tree yields logs through ordinary authoritative gathering");
    }
}

int main()
{
    TestContext test;
    try
    {
        CheckSchemaAndEdits(test);
        CheckHistory(test);
        CheckLiveRuntime(test);
    }
    catch (const std::exception &error) { test.Expect(false, std::string("Unexpected placement failure: ") + error.what()); }
    return test.Finish();
}
