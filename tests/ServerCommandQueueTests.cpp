#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Command/ServerCommandQueue.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Inventory/Inventory.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Player/Player.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/World.h"

#include <cstdint>
#include <type_traits>
#include <variant>

namespace
{
    Player *GetPlayer(World &world, int id)
    {
        return dynamic_cast<Player *>(
            world.GetEntityByID(id));
    }

    int FindSlot(const Inventory &inventory, ItemType type)
    {
        for (int index = 0;
             index < Inventory::SlotCount;
             ++index)
        {
            const InventorySlot &slot =
                inventory.GetSlots()[index];

            if (!slot.IsEmpty() &&
                slot.GetItemType() == type)
            {
                return index;
            }
        }

        return -1;
    }
}

int main()
{
    TestContext test;

    {
        ServerCommandQueue queue;
        test.Expect(queue.IsEmpty(), "Queue starts empty");
        test.ExpectEqual(queue.GetCount(), std::size_t{0},
                         "Empty queue has zero commands");
        test.Expect(!queue.PopNext().has_value(),
                    "Empty pop has no command");

        const std::uint64_t firstID = queue.Enqueue(
            MoveCommand{7, Position(3, 4)});
        const std::uint64_t secondID = queue.Enqueue(
            AttackCommand{7, 11});

        test.ExpectEqual(firstID, std::uint64_t{1},
                         "First ID is deterministic");
        test.ExpectEqual(secondID, std::uint64_t{2},
                         "IDs increment exactly once");
        test.ExpectEqual(queue.GetCount(), std::size_t{2},
                         "Enqueue records both variants");

        std::optional<ServerCommand> first = queue.PopNext();
        std::optional<ServerCommand> second = queue.PopNext();
        test.Expect(first.has_value() && second.has_value(),
                    "FIFO commands can be popped");
        test.Expect(std::holds_alternative<MoveCommand>(first->data),
                    "First variant retains move payload");
        test.Expect(std::holds_alternative<AttackCommand>(second->data),
                    "Second variant retains attack payload");
        test.ExpectEqual(
            std::get<MoveCommand>(first->data).destination.GetX(),
            3,
            "Move payload retains coordinates");
        test.Expect(queue.IsEmpty(), "Pop removes commands");

        for (int index = 0; index < 1000; ++index)
        {
            queue.Enqueue(MoveCommand{index, Position(index, 0)});
        }

        bool largeFIFOIsOrdered = true;
        for (int index = 0; index < 1000; ++index)
        {
            auto command = queue.PopNext();
            largeFIFOIsOrdered =
                largeFIFOIsOrdered && command.has_value() &&
                std::get<MoveCommand>(command->data).actorEntityID == index;
        }
        test.Expect(largeFIFOIsOrdered,
                    "Large sequence preserves global FIFO order");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        const int startTick = world.GetCurrentTick();
        const int startX = player->GetPosition().GetX();

        const std::uint64_t moveID = world.EnqueueCommand(
            MoveCommand{playerID, Position(startX + 1,
                                           player->GetPosition().GetY())});

        test.ExpectEqual(world.GetCurrentTick(), startTick,
                         "Enqueue does not advance tick");
        test.ExpectEqual(player->GetPosition().GetX(), startX,
                         "Enqueue does not mutate player");
        test.Expect(world.GetCommandProcessingResults().empty(),
                    "Enqueue does not publish a result");

        world.Update();
        const auto &results = world.GetCommandProcessingResults();
        test.ExpectEqual(results.size(), std::size_t{1},
                         "Update publishes exactly one result");
        test.ExpectEqual(results[0].commandID, moveID,
                         "Published result retains command ID");
        test.Expect(results[0].resultCode ==
                        CommandResultCode::ACCEPTED,
                    "Valid move is accepted");
        test.Expect(player->GetPosition().GetX() != startX,
                    "Move processes during the next update");

        world.Update();
        test.Expect(world.GetCommandProcessingResults().empty(),
                    "Next update clears old published results");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        const int monsterID = world.CreateMonster(
            3, 2, CombatRatings{8, 8, 8, 40});
        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(2, 2);

        const std::uint64_t attackID = world.EnqueueCommand(
            AttackCommand{playerID, monsterID});
        world.Update();

        const Action *action = world.GetActionForEntity(playerID);
        test.Expect(action != nullptr,
                    "Adjacent attack starts existing melee action");
        test.Expect(action != nullptr && action->GetStartTick() == 0,
                    "Command action starts at pre-increment tick");
        test.ExpectEqual(
            world.GetCommandProcessingResults()[0].commandID,
            attackID,
            "Attack result is published");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        const int monsterID = world.CreateMonster(
            4, 4, CombatRatings{8, 8, 8, 40});
        Player *player = GetPlayer(world, playerID);
        const int initialHealth = player->GetCurrentHealth();

        world.EnqueueCommand(MoveCommand{-999, Position(2, 2)});
        world.EnqueueCommand(MoveCommand{monsterID, Position(2, 2)});
        world.EnqueueCommand(MoveCommand{playerID, Position(-1, 2)});
        world.EnqueueCommand(AttackCommand{playerID, 999999});
        world.EnqueueCommand(UnequipItemCommand{
            playerID,
            static_cast<EquipmentSlotType>(999)});
        world.Update();

        const auto &results = world.GetCommandProcessingResults();
        test.ExpectEqual(results.size(), std::size_t{5},
                         "Each rejected command has one result");
        test.Expect(results[0].resultCode ==
                        CommandResultCode::INVALID_ACTOR,
                    "Missing actor is rejected");
        test.Expect(results[1].resultCode ==
                        CommandResultCode::INVALID_ACTOR,
                    "Non-player actor is rejected");
        test.Expect(results[2].resultCode ==
                        CommandResultCode::INVALID_COMMAND_DATA &&
                    results[3].resultCode ==
                        CommandResultCode::INVALID_COMMAND_DATA &&
                    results[4].resultCode ==
                        CommandResultCode::INVALID_COMMAND_DATA,
                    "Malformed coordinates, target and enum are rejected");
        test.ExpectEqual(player->GetCurrentHealth(), initialHealth,
                         "Rejections leave gameplay state unchanged");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        player->ApplyDamage(8);
        player->GetInventory().AddItem(ItemType::COOKED_MEAT, 2);
        const int foodSlot = FindSlot(
            player->GetInventory(), ItemType::COOKED_MEAT);

        world.EnqueueCommand(
            UseInventoryItemCommand{playerID, foodSlot});
        world.EnqueueCommand(
            UseInventoryItemCommand{playerID, foodSlot});
        world.Update();

        const auto &results = world.GetCommandProcessingResults();
        test.ExpectEqual(results.size(), std::size_t{2},
                         "Two food commands process sequentially");
        test.Expect(results[0].commandID < results[1].commandID,
                    "Result order matches FIFO command order");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COOKED_MEAT),
            1,
            "Second food use observes health changed by the first");
        test.Expect(
            results[0].resultCode == CommandResultCode::ACCEPTED &&
                results[1].resultCode ==
                    CommandResultCode::GAMEPLAY_REJECTED,
            "Full-health second food command is rejected without consuming");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(3, 3);
        player->GetInventory().AddItem(ItemType::COAL, 2);
        const int coalSlot = FindSlot(player->GetInventory(), ItemType::COAL);

        world.EnqueueCommand(DropInventoryItemCommand{playerID, coalSlot, 1});
        world.Update();
        test.Expect(world.GetCommandProcessingResults()[0].resultCode ==
                CommandResultCode::ACCEPTED &&
                world.GetGroundItems().size() == 1 &&
                world.GetGroundItems()[0].amount == 1,
            "Dropping removes an inventory quantity and creates an authoritative ground item");

        const int groundItemID = world.GetGroundItems()[0].id;
        world.EnqueueCommand(TakeGroundItemCommand{playerID, groundItemID});
        world.Update();
        test.Expect(world.GetCommandProcessingResults()[0].resultCode ==
                CommandResultCode::ACCEPTED &&
                world.GetGroundItems().empty() &&
                player->GetInventory().GetItemAmount(ItemType::COAL) == 2,
            "Taking an adjacent ground item restores it atomically to inventory");

        world.EnqueueCommand(DropInventoryItemCommand{playerID, -1, 1});
        world.EnqueueCommand(TakeGroundItemCommand{playerID, 999999});
        world.Update();
        test.Expect(
            world.GetCommandProcessingResults()[0].resultCode ==
                CommandResultCode::INVALID_COMMAND_DATA &&
            world.GetCommandProcessingResults()[1].resultCode ==
                CommandResultCode::INVALID_COMMAND_DATA,
            "Malformed drop and pickup requests are rejected without mutation");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(19, 3);
        CraftingStation *bankStation = world.GetStationAt(20, 3);
        test.Expect(bankStation != nullptr &&
                bankStation->GetStationType() == StationType::BANK,
            "Starter region exposes the authored bank station");
        world.EnqueueCommand(InteractCommand{playerID,
            InteractionTargetType::STATION, bankStation->GetID(), Position(20, 3)});
        world.Update();
        const ActiveBankSession *session = world.GetBankSession(playerID);
        test.Expect(session != nullptr &&
                session->sessionId != InvalidBankSessionId,
            "Approaching the bank opens a stable per-player session");

        player->GetInventory().AddItem(ItemType::COINS, 12);
        const int coinSlot = FindSlot(player->GetInventory(), ItemType::COINS);
        const BankSessionId sessionId = session->sessionId;
        const std::uint64_t revision = player->GetBank().GetRevision();
        world.EnqueueCommand(BankDepositCommand{
            playerID, sessionId, revision, coinSlot, 5});
        world.EnqueueCommand(BankDepositCommand{
            playerID, sessionId, revision, coinSlot, 5});
        world.Update();
        test.Expect(
            world.GetCommandProcessingResults()[0].resultCode ==
                CommandResultCode::ACCEPTED &&
            world.GetCommandProcessingResults()[1].resultCode ==
                CommandResultCode::GAMEPLAY_REJECTED,
            "A repeated stale bank request cannot duplicate a transaction");
        test.ExpectEqual(player->GetBank().GetItemAmount(ItemType::COINS), 5,
            "Only the first revision-matched deposit commits");

        world.EnqueueCommand(BankCloseCommand{playerID, sessionId});
        world.Update();
        test.Expect(world.GetBankSession(playerID) == nullptr,
            "Closing a matching bank session invalidates further requests");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        player->GetTutorialState().TryRestore(TutorialStep::GATHER_LOG, false);
        player->GetInventory().AddItem(ItemType::LOG, 1); world.Update();
        test.Expect(player->GetTutorialState().GetStep() == TutorialStep::MAKE_PLANKS,
            "Tutorial advances from authoritative gathered inventory");
        player->GetInventory().AddItem(ItemType::WOODEN_PLANKS, 1); world.Update();
        player->GetInventory().AddItem(ItemType::COPPER_ORE, 1);
        player->GetInventory().AddItem(ItemType::TIN_ORE, 1); world.Update();
        player->GetInventory().AddItem(ItemType::BRONZE_BAR, 1); world.Update();
        player->GetInventory().AddItem(ItemType::BRONZE_SWORD, 1); world.Update();
        test.Expect(player->GetTutorialState().GetStep() == TutorialStep::EQUIP_WEAPON,
            "Crafting milestones advance in the authored sequence");
        const int swordSlot = FindSlot(player->GetInventory(), ItemType::BRONZE_SWORD);
        world.TryEquipInventoryItem(playerID, swordSlot); world.Update();
        test.Expect(player->GetTutorialState().GetStep() == TutorialStep::SAFE_COMBAT,
            "Equipping a real weapon advances preparation onboarding");
        player->GetTutorialState().TryRestore(TutorialStep::LOOT_AND_BANK, false);
        player->GetInventory().AddItem(ItemType::COINS, 1);
        const int coinSlot = FindSlot(player->GetInventory(), ItemType::COINS);
        player->GetBank().TryDeposit(player->GetInventory(), coinSlot, 1);
        world.Update();
        test.Expect(player->GetTutorialState().GetStep() ==
                TutorialStep::RETURN_AND_CHOOSE,
            "A committed bank deposit advances the loot-and-bank lesson");
    }

    return test.Finish();
}
