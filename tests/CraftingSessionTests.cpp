#include "TestSupport.h"

#include "../src/Command/CommandProcessingResult.h"
#include "../src/Player/Player.h"
#include "../src/Recipe/CraftingInteractionModel.h"
#include "../src/World/World.h"

namespace
{
CommandResultCode ResultFor(const World &world, std::uint64_t commandID)
{
    for (const CommandProcessingResult &result :
         world.GetCommandProcessingResults())
        if (result.commandID == commandID)
            return result.resultCode;
    return CommandResultCode::GAMEPLAY_REJECTED;
}

Player *CreatePlayer(World &world)
{
    return dynamic_cast<Player *>(world.GetEntityByID(world.CreatePlayer()));
}

bool OpenCraftingTable(World &world, Player &player)
{
    CraftingStation *station = world.GetStationAt(17, 9);
    if (station == nullptr) return false;
    player.GetPosition().SetPosition(16, 9);
    world.QueueStationInteraction(player.GetID(), station->GetID());
    world.Update();
    StationType type = StationType::NONE;
    return world.ConsumeOpenedStation(player.GetID(), type) &&
           type == StationType::CRAFTING_TABLE;
}
}

int main()
{
    TestContext test;

    {
        World world;
        Player *player = CreatePlayer(world);
        test.Expect(player != nullptr && OpenCraftingTable(world, *player),
                    "Adjacent Crafting Table creates an interaction");
        const CraftingSessionSnapshot *opened =
            world.GetCraftingSession(player->GetID());
        test.Expect(opened != nullptr &&
                        opened->sessionId != InvalidCraftingSessionId &&
                        opened->actorEntityID == player->GetID() &&
                        opened->revision == 0,
                    "Crafting Table publishes an empty authoritative session");

        const CraftingSessionId sessionId = opened->sessionId;
        player->GetInventory().AddItem(ItemType::COINS, 7);
        player->GetInventory().AddItem(ItemType::LOG, 1);
        ReplaceCraftingSessionStateCommand replacement{};
        replacement.actorEntityID = player->GetID();
        replacement.sessionId = sessionId;
        replacement.expectedRevision = 0;
        replacement.cursor.SetItem(ItemType::COINS, 5);
        replacement.inputSlots[4].SetItem(ItemType::LOG, 1);
        const std::uint64_t replaceID = world.EnqueueCommand(replacement);
        world.Update();

        const CraftingSessionSnapshot *updated =
            world.GetCraftingSession(player->GetID());
        test.Expect(ResultFor(world, replaceID) == CommandResultCode::ACCEPTED &&
                        updated != nullptr && updated->revision == 1,
                    "Matching actor, session and revision replace state once");
        test.Expect(updated->cursor.GetAmount() == 5 &&
                        updated->inputSlots[4].GetItemType() == ItemType::LOG &&
                        player->GetInventory().GetItemAmount(ItemType::COINS) == 2 &&
                        player->GetInventory().GetItemAmount(ItemType::LOG) == 0,
                    "Moving items into the session conserves exact quantities");

        const std::uint64_t staleID = world.EnqueueCommand(replacement);
        world.Update();
        updated = world.GetCraftingSession(player->GetID());
        test.Expect(ResultFor(world, staleID) == CommandResultCode::GAMEPLAY_REJECTED &&
                        updated != nullptr && updated->revision == 1,
                    "Stale revision cannot replay a state replacement");

        ReplaceCraftingSessionStateCommand returnItems = replacement;
        returnItems.expectedRevision = 1;
        returnItems.cursor.Clear();
        returnItems.inputSlots = {};
        const std::uint64_t returnID = world.EnqueueCommand(returnItems);
        world.Update();
        test.Expect(ResultFor(world, returnID) == CommandResultCode::ACCEPTED &&
                        player->GetInventory().GetItemAmount(ItemType::COINS) == 7 &&
                        player->GetInventory().GetItemAmount(ItemType::LOG) == 1,
                    "Returning session items commits atomically to inventory");

        test.Expect(OpenCraftingTable(world, *player) &&
                        world.GetCraftingSession(player->GetID())->sessionId == sessionId,
                    "Reopening the same active table preserves session identity");
    }

    {
        World world;
        Player *player = CreatePlayer(world);
        OpenCraftingTable(world, *player);
        player->GetInventory().AddItem(ItemType::COINS, 9);
        const CraftingSessionSnapshot opened = *world.GetCraftingSession(player->GetID());
        CraftingInteractionModel model(player->GetInventory(), opened);
        int coinSlot = -1;
        for (int index = 0; index < Inventory::SlotCount; ++index)
            if (model.GetSlots()[index].GetItemType() == ItemType::COINS) coinSlot = index;
        test.Expect(coinSlot >= 0 && model.TryRightClick(coinSlot) &&
                        model.GetCursor().GetAmount() == 5,
                    "Shared model right-click picks up rounded-up half");
        test.Expect(model.TryRightClick(Inventory::SlotCount + 4) &&
                        model.GetSlots()[Inventory::SlotCount + 4].GetAmount() == 1 &&
                        model.GetCursor().GetAmount() == 4,
                    "Shared model places one item into the crafting grid");
        const std::uint64_t commandID = world.EnqueueCommand(model.BuildCommand());
        world.Update();
        test.Expect(ResultFor(world, commandID) == CommandResultCode::ACCEPTED &&
                        world.GetCraftingSession(player->GetID())->cursor.GetAmount() == 4 &&
                        world.GetCraftingSession(player->GetID())->inputSlots[4].GetAmount() == 1 &&
                        player->GetInventory().GetItemAmount(ItemType::COINS) == 4,
                    "Shared model command commits through authoritative World validation");
    }

    {
        World world;
        Player *owner = CreatePlayer(world);
        Player *other = CreatePlayer(world);
        OpenCraftingTable(world, *owner);
        const CraftingSessionSnapshot original =
            *world.GetCraftingSession(owner->GetID());

        ReplaceCraftingSessionStateCommand wrongActor{};
        wrongActor.actorEntityID = other->GetID();
        wrongActor.sessionId = original.sessionId;
        wrongActor.expectedRevision = original.revision;
        wrongActor.cursor.SetItem(ItemType::COINS, 1);
        const std::uint64_t wrongActorID = world.EnqueueCommand(wrongActor);

        ReplaceCraftingSessionStateCommand wrongSession = wrongActor;
        wrongSession.actorEntityID = owner->GetID();
        wrongSession.sessionId = original.sessionId + 1;
        const std::uint64_t wrongSessionID = world.EnqueueCommand(wrongSession);
        world.Update();
        test.Expect(ResultFor(world, wrongActorID) == CommandResultCode::GAMEPLAY_REJECTED &&
                        ResultFor(world, wrongSessionID) == CommandResultCode::GAMEPLAY_REJECTED,
                    "Wrong actor and wrong session cannot manipulate another session");
        test.Expect(world.GetCraftingSession(owner->GetID())->revision == 0,
                    "Rejected identities leave authoritative state unchanged");
    }

    {
        World world;
        Player *player = CreatePlayer(world);
        OpenCraftingTable(world, *player);
        const CraftingSessionSnapshot opened =
            *world.GetCraftingSession(player->GetID());
        player->GetInventory().AddItem(ItemType::COINS, 3);
        ReplaceCraftingSessionStateCommand place{};
        place.actorEntityID = player->GetID();
        place.sessionId = opened.sessionId;
        place.expectedRevision = opened.revision;
        place.cursor.SetItem(ItemType::COINS, 3);
        world.EnqueueCommand(place);
        world.Update();

        const std::uint64_t closeID = world.EnqueueCommand(
            CloseStationCommand{player->GetID()});
        world.Update();
        test.Expect(ResultFor(world, closeID) == CommandResultCode::ACCEPTED &&
                        world.GetCraftingSession(player->GetID()) == nullptr,
                    "Normal close settles and removes the authoritative session");
        test.Expect(player->GetInventory().GetItemAmount(ItemType::COINS) == 3 &&
                        !world.HasActiveStation(player->GetID()),
                    "Normal close returns every held item before closing");
    }

    {
        World world;
        Player *player = CreatePlayer(world);
        OpenCraftingTable(world, *player);
        const CraftingSessionSnapshot opened =
            *world.GetCraftingSession(player->GetID());
        player->GetInventory().AddItem(ItemType::LOG, 1);
        ReplaceCraftingSessionStateCommand malformed{};
        malformed.actorEntityID = player->GetID();
        malformed.sessionId = opened.sessionId;
        malformed.expectedRevision = opened.revision;
        malformed.cursor.SetItem(ItemType::LOG, 2);
        const std::uint64_t malformedID = world.EnqueueCommand(malformed);
        world.Update();
        test.Expect(ResultFor(world, malformedID) ==
                            CommandResultCode::GAMEPLAY_REJECTED &&
                        world.GetCraftingSession(player->GetID())->revision == 0 &&
                        player->GetInventory().GetItemAmount(ItemType::LOG) == 1,
                    "Malformed non-stackable quantities reject without mutation");

        player->ApplyDamage(1000);
        const std::uint64_t deadID = world.EnqueueCommand(malformed);
        world.Update();
        test.Expect(ResultFor(world, deadID) == CommandResultCode::GAMEPLAY_REJECTED &&
                        world.GetCraftingSession(player->GetID()) == nullptr,
                    "Dead actors cannot mutate sessions and the session invalidates safely");
    }

    {
        World world;
        Player *player = CreatePlayer(world);
        OpenCraftingTable(world, *player);
        const CraftingSessionSnapshot opened =
            *world.GetCraftingSession(player->GetID());
        player->GetInventory().AddItem(ItemType::LOG, 1);
        ReplaceCraftingSessionStateCommand place{};
        place.actorEntityID = player->GetID();
        place.sessionId = opened.sessionId;
        place.expectedRevision = 0;
        place.inputSlots[0].SetItem(ItemType::LOG, 1);
        world.EnqueueCommand(place);
        world.Update();

        std::array<InventorySlot, Inventory::SlotCount> full{};
        for (InventorySlot &slot : full) slot.SetItem(ItemType::LOG, 1);
        test.Expect(player->GetInventory().TryReplaceSlotsAtomically(full),
                    "Fixture fills every inventory slot canonically");

        const std::uint64_t moveID = world.EnqueueCommand(
            MoveCommand{player->GetID(), Position(1, 1)});
        world.Update();
        const CraftingSessionSnapshot *retained =
            world.GetCraftingSession(player->GetID());
        test.Expect(ResultFor(world, moveID) == CommandResultCode::ACCEPTED &&
                        retained != nullptr &&
                        retained->inputSlots[0].GetItemType() == ItemType::LOG,
                    "Forced invalidation safely retains items when inventory is full");
        test.Expect(!world.HasActiveStation(player->GetID()),
                    "Invalidated retained session cannot keep station authority");
    }

    {
        World world;
        Player *player = CreatePlayer(world);
        OpenCraftingTable(world, *player);
        player->GetInventory().AddItem(ItemType::COINS, 9);
        const CraftingSessionSnapshot opened = *world.GetCraftingSession(player->GetID());
        ReplaceCraftingInteractionStateCommand interaction{};
        interaction.actorEntityID = player->GetID();
        interaction.sessionId = opened.sessionId;
        interaction.expectedRevision = opened.revision;
        interaction.inventorySlots = player->GetInventory().GetSlots();
        int coinSlot = -1;
        for (int index = 0; index < Inventory::SlotCount; ++index)
            if (interaction.inventorySlots[index].GetItemType() == ItemType::COINS)
                coinSlot = index;
        if (coinSlot >= 0) interaction.inventorySlots[coinSlot].SetItem(ItemType::COINS, 2);
        interaction.cursor.SetItem(ItemType::COINS, 3);
        interaction.inputSlots[4].SetItem(ItemType::COINS, 4);
        const std::uint64_t accepted = world.EnqueueCommand(interaction);
        world.Update();
        const CraftingSessionSnapshot *updated = world.GetCraftingSession(player->GetID());
        test.Expect(ResultFor(world, accepted) == CommandResultCode::ACCEPTED &&
                        updated != nullptr && updated->revision == 1 &&
                        player->GetInventory().GetItemAmount(ItemType::COINS) == 2 &&
                        updated->cursor.GetAmount() == 3 && updated->inputSlots[4].GetAmount() == 4,
                    "Shared inventory, cursor and crafting state commits atomically");

        ReplaceCraftingInteractionStateCommand duplicate = interaction;
        if (coinSlot >= 0) duplicate.inventorySlots[coinSlot].SetItem(ItemType::COINS, 3);
        const std::uint64_t duplicateID = world.EnqueueCommand(duplicate);
        world.Update();
        test.Expect(ResultFor(world, duplicateID) == CommandResultCode::GAMEPLAY_REJECTED &&
                        player->GetInventory().GetItemAmount(ItemType::COINS) == 2 &&
                        world.GetCraftingSession(player->GetID())->revision == 1,
                    "Stale or item-creating shared replacements leave all containers unchanged");
    }

    {
        Inventory inventory;
        inventory.AddItem(ItemType::COINS, 12);
        inventory.AddItem(ItemType::LOG, 1);
        CraftingSessionSnapshot session{};
        session.sessionId = 42;
        session.actorEntityID = 7;
        CraftingInteractionModel model(inventory, session);
        int coinSlot = -1;
        int logSlot = -1;
        int emptySlot = -1;
        for (int index = 0; index < Inventory::SlotCount; ++index)
        {
            if (model.GetSlots()[index].GetItemType() == ItemType::COINS) coinSlot = index;
            if (model.GetSlots()[index].GetItemType() == ItemType::LOG) logSlot = index;
            if (model.GetSlots()[index].IsEmpty() && emptySlot < 0) emptySlot = index;
        }
        test.Expect(model.TryLeftClick(coinSlot),
                    "Left click picks up an inventory stack into the shared cursor");
        const std::array<int, 3> leftDrag{
            Inventory::SlotCount, Inventory::SlotCount + 1, Inventory::SlotCount + 2};
        test.Expect(model.TryLeftDrag(leftDrag) && model.GetCursor().IsEmpty() &&
                        model.GetSlots()[Inventory::SlotCount].GetAmount() == 4,
                    "Left drag distributes evenly across crafting slots");
        test.Expect(model.TryRightClick(Inventory::SlotCount) &&
                        model.GetCursor().GetAmount() == 2,
                    "Right click splits a crafting stack into the shared cursor");
        const std::array<int, 2> rightDrag{
            Inventory::SlotCount + 3, emptySlot};
        test.Expect(model.TryRightDrag(rightDrag) && model.GetCursor().IsEmpty(),
                    "Right drag places one across crafting and inventory slots");
        test.Expect(model.TryShiftTransfer(logSlot),
                    "Shift transfer moves inventory items into the crafting surface");
        test.Expect(model.TryLeftClick(Inventory::SlotCount + 1) &&
                        model.TryCollectMatching(),
                    "Double-click model collection gathers matching stacks across both containers");
    }

    return test.Finish();
}
