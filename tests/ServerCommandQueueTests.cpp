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

    return test.Finish();
}
