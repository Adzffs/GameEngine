#include "TestSupport.h"

#include "../src/Command/CommandProcessingResult.h"
#include "../src/Player/Player.h"
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

    return test.Finish();
}
