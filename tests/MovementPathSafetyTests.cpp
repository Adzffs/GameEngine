#include "TestSupport.h"

#include "../src/Action/ActionCancelReason.h"
#include "../src/Action/ActionType.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Movement/Movement.h"
#include "../src/Movement/MovementDestinationRequest.h"
#include "../src/Movement/MovementRequest.h"
#include "../src/Player/Player.h"
#include "../src/Reward/RewardTableType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/Event/ActionCancelledEvent.h"
#include "../src/World/Tile/TileType.h"
#include "../src/World/World.h"

#include <array>
#include <cstdlib>
#include <optional>
#include <string>
#include <variant>

struct WorldTestAccess
{
    static bool HasActiveMovementPath(
        const World &world,
        int entityID)
    {
        return world.activeMovementPaths.find(entityID) !=
               world.activeMovementPaths.end();
    }

    static int GetRemainingStepCount(
        const World &world,
        int entityID)
    {
        auto iterator =
            world.activeMovementPaths.find(entityID);

        if (iterator == world.activeMovementPaths.end())
        {
            return 0;
        }

        return static_cast<int>(
            iterator->second.remainingSteps.size());
    }

    static std::optional<PathStep> GetNextStep(
        const World &world,
        int entityID)
    {
        auto iterator =
            world.activeMovementPaths.find(entityID);

        if (iterator == world.activeMovementPaths.end() ||
            iterator->second.remainingSteps.empty())
        {
            return std::nullopt;
        }

        return iterator->second.remainingSteps.front();
    }

    static std::optional<PathStep> GetDestination(
        const World &world,
        int entityID)
    {
        auto iterator =
            world.activeMovementPaths.find(entityID);

        if (iterator == world.activeMovementPaths.end())
        {
            return std::nullopt;
        }

        return iterator->second.destination;
    }
};

namespace
{
    Player *GetPlayer(
        World &world,
        int entityID)
    {
        return dynamic_cast<Player *>(
            world.GetEntityByID(entityID));
    }

    Monster *GetMonster(
        World &world,
        int entityID)
    {
        return dynamic_cast<Monster *>(
            world.GetEntityByID(entityID));
    }

    int ManhattanDistance(
        int firstX,
        int firstY,
        int secondX,
        int secondY)
    {
        return std::abs(firstX - secondX) +
               std::abs(firstY - secondY);
    }

    void AdvanceTicks(
        World &world,
        int ticks)
    {
        for (int tick = 0; tick < ticks; ++tick)
        {
            world.Update();
        }
    }

    int FindInventorySlot(
        const Inventory &inventory,
        ItemType itemType)
    {
        const auto &slots = inventory.GetSlots();

        for (int index = 0;
             index < static_cast<int>(slots.size());
             ++index)
        {
            if (!slots[index].IsEmpty() &&
                slots[index].GetItemType() == itemType)
            {
                return index;
            }
        }

        return -1;
    }

    bool EquipItem(
        World &world,
        int playerID,
        ItemType itemType)
    {
        Player *player = GetPlayer(world, playerID);

        if (player == nullptr)
        {
            return false;
        }

        int slotIndex = FindInventorySlot(
            player->GetInventory(),
            itemType);

        return slotIndex >= 0 &&
               world.TryEquipInventoryItem(
                   playerID,
                   slotIndex);
    }

    bool ContainsCancellationReason(
        const std::vector<ActionLifecycleEvent> &events,
        ActionCancelReason reason)
    {
        for (const ActionLifecycleEvent &event : events)
        {
            const ActionCancelledEvent *cancelledEvent =
                std::get_if<ActionCancelledEvent>(
                    &event.data);

            if (cancelledEvent != nullptr &&
                cancelledEvent->reason == reason)
            {
                return true;
            }
        }

        return false;
    }

    void TestMovementPrimitive(TestContext &test)
    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        Map &map = world.GetMap();

        const std::array<std::array<int, 4>, 4> validMoves{{
            {0, -1, 30, 29},
            {0, 1, 30, 31},
            {1, 0, 31, 30},
            {-1, 0, 29, 30}}};

        for (const auto &move : validMoves)
        {
            player->GetPosition().SetPosition(30, 30);

            test.Expect(
                Movement::Move(
                    *player,
                    map,
                    move[0],
                    move[1]),
                "One orthogonal tile movement succeeds");
            test.ExpectEqual(
                player->GetPosition().GetX(),
                move[2],
                "Successful primitive movement updates X exactly once");
            test.ExpectEqual(
                player->GetPosition().GetY(),
                move[3],
                "Successful primitive movement updates Y exactly once");
        }

        auto expectRejectedMove =
            [&](int startX,
                int startY,
                int changeX,
                int changeY,
                const std::string &description)
        {
            player->GetPosition().SetPosition(
                startX,
                startY);

            test.Expect(
                !Movement::Move(
                    *player,
                    map,
                    changeX,
                    changeY),
                description + " is rejected");
            test.ExpectEqual(
                player->GetPosition().GetX(),
                startX,
                description + " preserves X");
            test.ExpectEqual(
                player->GetPosition().GetY(),
                startY,
                description + " preserves Y");
        };

        expectRejectedMove(30, 30, 0, 0, "Same-tile movement");
        expectRejectedMove(30, 30, 2, 0, "Two-tile horizontal movement");
        expectRejectedMove(30, 30, 0, -2, "Two-tile vertical movement");
        expectRejectedMove(30, 30, 1, 1, "Diagonal movement");
        expectRejectedMove(0, 0, -1, 0, "Out-of-bounds movement");

        map.SetTileType(31, 30, TileType::WALL);
        expectRejectedMove(30, 30, 1, 0, "Blocked-tile movement");
    }

    void TestActivePathProgress(TestContext &test)
    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(30, 30);

        world.QueueMovementDestination(
            MovementDestinationRequest(
                playerID,
                34,
                30));

        world.Update();

        test.ExpectEqual(
            player->GetPosition().GetX(),
            31,
            "The first path update executes exactly one tile step");
        test.ExpectEqual(
            WorldTestAccess::GetRemainingStepCount(
                world,
                playerID),
            3,
            "Successful movement removes exactly one stored step");

        int priorX = player->GetPosition().GetX();
        int priorY = player->GetPosition().GetY();
        world.Update();

        test.ExpectEqual(
            ManhattanDistance(
                priorX,
                priorY,
                player->GetPosition().GetX(),
                player->GetPosition().GetY()),
            1,
            "An active path moves no more than one tile per update");
        test.ExpectEqual(
            WorldTestAccess::GetRemainingStepCount(
                world,
                playerID),
            2,
            "A second successful update removes one more step");

        AdvanceTicks(world, 2);

        test.ExpectEqual(
            player->GetPosition().GetX(),
            34,
            "The valid active path reaches its X destination");
        test.ExpectEqual(
            player->GetPosition().GetY(),
            30,
            "The valid active path reaches its Y destination");
        test.Expect(
            !WorldTestAccess::HasActiveMovementPath(
                world,
                playerID),
            "The active path clears after its final successful step");
    }

    void TestSuccessfulDynamicRecalculation(TestContext &test)
    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(30, 40);

        world.QueueMovementDestination(
            MovementDestinationRequest(
                playerID,
                35,
                40));
        world.Update();

        std::optional<PathStep> blockedStep =
            WorldTestAccess::GetNextStep(
                world,
                playerID);

        test.Expect(
            blockedStep.has_value(),
            "A future path step exists before dynamic obstruction");

        if (blockedStep.has_value())
        {
            world.GetMap().SetTileType(
                blockedStep->x,
                blockedStep->y,
                TileType::WALL);
        }

        const int positionBeforeFailureX =
            player->GetPosition().GetX();
        const int positionBeforeFailureY =
            player->GetPosition().GetY();

        world.Update();

        test.ExpectEqual(
            player->GetPosition().GetX(),
            positionBeforeFailureX,
            "A dynamically blocked step does not move X during recalculation");
        test.ExpectEqual(
            player->GetPosition().GetY(),
            positionBeforeFailureY,
            "A dynamically blocked step does not move Y during recalculation");
        test.Expect(
            WorldTestAccess::HasActiveMovementPath(
                world,
                playerID),
            "A valid detour replaces the obstructed path");

        std::optional<PathStep> destination =
            WorldTestAccess::GetDestination(
                world,
                playerID);
        std::optional<PathStep> replacementStep =
            WorldTestAccess::GetNextStep(
                world,
                playerID);

        test.Expect(
            destination.has_value() &&
                destination->x == 35 &&
                destination->y == 40,
            "Recalculation retains the original final destination");
        test.Expect(
            replacementStep.has_value() &&
                ManhattanDistance(
                    positionBeforeFailureX,
                    positionBeforeFailureY,
                    replacementStep->x,
                    replacementStep->y) == 1,
            "The replacement path begins with an orthogonally adjacent step");

        world.Update();

        test.ExpectEqual(
            ManhattanDistance(
                positionBeforeFailureX,
                positionBeforeFailureY,
                player->GetPosition().GetX(),
                player->GetPosition().GetY()),
            1,
            "The update after recalculation executes one legal detour step");
        test.Expect(
            player->GetPosition().GetX() != 33 ||
                player->GetPosition().GetY() != 40,
            "A later stored step is never executed as a multi-tile jump");

        for (int tick = 0;
             tick < 12 &&
             WorldTestAccess::HasActiveMovementPath(
                 world,
                 playerID);
             ++tick)
        {
            int priorX = player->GetPosition().GetX();
            int priorY = player->GetPosition().GetY();

            world.Update();

            test.ExpectEqual(
                ManhattanDistance(
                    priorX,
                    priorY,
                    player->GetPosition().GetX(),
                    player->GetPosition().GetY()),
                1,
                "Every recalculated detour update executes one tile step");
        }

        test.ExpectEqual(
            player->GetPosition().GetX(),
            35,
            "The recalculated path reaches its original X destination");
        test.ExpectEqual(
            player->GetPosition().GetY(),
            40,
            "The recalculated path reaches its original Y destination");
    }

    void TestFailedDynamicRecalculation(TestContext &test)
    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(50, 50);

        world.QueueMovementDestination(
            MovementDestinationRequest(
                playerID,
                54,
                50));
        world.Update();

        const int trappedX = player->GetPosition().GetX();
        const int trappedY = player->GetPosition().GetY();

        const std::array<std::array<int, 2>, 8> neighborOffsets{{
            {-1, -1},
            {0, -1},
            {1, -1},
            {-1, 0},
            {1, 0},
            {-1, 1},
            {0, 1},
            {1, 1}}};

        for (const auto &offset : neighborOffsets)
        {
            world.GetMap().SetTileType(
                trappedX + offset[0],
                trappedY + offset[1],
                TileType::WALL);
        }

        world.Update();

        test.ExpectEqual(
            player->GetPosition().GetX(),
            trappedX,
            "Failed recalculation preserves the entity X position");
        test.ExpectEqual(
            player->GetPosition().GetY(),
            trappedY,
            "Failed recalculation preserves the entity Y position");
        test.Expect(
            !WorldTestAccess::HasActiveMovementPath(
                world,
                playerID),
            "A path is cancelled when no recalculated route exists");

        world.Update();

        test.ExpectEqual(
            player->GetPosition().GetX(),
            trappedX,
            "A cancelled path is not retried repeatedly on later updates");
        test.ExpectEqual(
            player->GetPosition().GetY(),
            trappedY,
            "Cancellation executes no later stored path step");

        world.GetMap().SetTileType(
            trappedX - 1,
            trappedY,
            TileType::GRASS);
        world.QueueMovementDestination(
            MovementDestinationRequest(
                playerID,
                trappedX - 2,
                trappedY));

        world.Update();
        world.Update();

        test.ExpectEqual(
            player->GetPosition().GetX(),
            trappedX - 2,
            "A newly queued path works after prior cancellation");
        test.ExpectEqual(
            player->GetPosition().GetY(),
            trappedY,
            "New movement after cancellation remains on its legal row");
    }

    void TestDestinationAndInteractionApproaches(TestContext &test)
    {
        {
            World world;
            int playerID = world.CreatePlayer();
            Player *player = GetPlayer(world, playerID);
            player->GetPosition().SetPosition(60, 10);

            world.QueueMovementDestination(
                MovementDestinationRequest(
                    playerID,
                    63,
                    13));

            for (int tick = 0; tick < 6; ++tick)
            {
                int priorX = player->GetPosition().GetX();
                int priorY = player->GetPosition().GetY();
                world.Update();

                test.ExpectEqual(
                    ManhattanDistance(
                        priorX,
                        priorY,
                        player->GetPosition().GetX(),
                        player->GetPosition().GetY()),
                    1,
                    "Destination walking executes only orthogonal click-to-walk steps");
            }

            test.ExpectEqual(
                player->GetPosition().GetX(),
                63,
                "Destination walking reaches a valid X coordinate");
            test.ExpectEqual(
                player->GetPosition().GetY(),
                13,
                "Destination walking reaches a valid Y coordinate");
        }

        {
            World world;
            int playerID = world.CreatePlayer();
            Player *player = GetPlayer(world, playerID);
            ResourceNode *resource = world.GetResourceAt(5, 5);

            player->GetPosition().SetPosition(2, 5);

            test.Expect(
                resource != nullptr &&
                    EquipItem(
                        world,
                        playerID,
                        ItemType::BRONZE_AXE),
                "Resource approach setup equips the required axe");

            if (resource != nullptr)
            {
                world.QueueResourceInteraction(
                    playerID,
                    resource->GetID());
                world.QueueMovementDestination(
                    MovementDestinationRequest(
                        playerID,
                        resource->GetX(),
                        resource->GetY()));
                AdvanceTicks(world, 2);

                test.Expect(
                    std::abs(player->GetPosition().GetX() - resource->GetX()) <= 1 &&
                        std::abs(player->GetPosition().GetY() - resource->GetY()) <= 1,
                    "Resource approach stops in the existing interaction range");
                test.Expect(
                    player->GetPosition().GetX() != resource->GetX() ||
                        player->GetPosition().GetY() != resource->GetY(),
                    "Resource approach does not enter the blocked resource tile");

                const Action *action =
                    world.GetActionForEntity(playerID);

                test.Expect(
                    action != nullptr &&
                        action->GetType() == ActionType::GATHERING,
                    "Successful resource approach still starts gathering");
            }
        }

        {
            World world;
            int playerID = world.CreatePlayer();
            Player *player = GetPlayer(world, playerID);
            CraftingStation *station =
                world.GetStationAt(14, 9);

            player->GetPosition().SetPosition(14, 6);

            test.Expect(
                station != nullptr,
                "Furnace approach setup finds the development station");

            if (station != nullptr)
            {
                world.QueueStationInteraction(
                    playerID,
                    station->GetID());
                world.QueueMovementDestination(
                    MovementDestinationRequest(
                        playerID,
                        station->GetX(),
                        station->GetY()));
                AdvanceTicks(world, 2);

                test.Expect(
                    std::abs(player->GetPosition().GetX() - station->GetX()) <= 1 &&
                        std::abs(player->GetPosition().GetY() - station->GetY()) <= 1,
                    "Furnace approach stops in the existing interaction range");
                test.Expect(
                    player->GetPosition().GetX() != station->GetX() ||
                        player->GetPosition().GetY() != station->GetY(),
                    "Furnace approach does not enter the blocked station tile");

                StationType openedStationType = StationType::NONE;

                test.Expect(
                    world.ConsumeOpenedStation(
                        playerID,
                        openedStationType) &&
                        openedStationType == StationType::FURNACE,
                    "Successful furnace approach preserves station interaction behavior");
            }
        }

        {
            World world;
            int playerID = world.CreatePlayer();
            int monsterID = world.CreateMonster(
                74,
                21,
                CombatRatings{8, 8, 8, 40});
            Player *player = GetPlayer(world, playerID);
            Monster *monster = GetMonster(world, monsterID);

            player->GetPosition().SetPosition(70, 20);

            test.Expect(
                world.QueueMeleeEngagementRequest(
                    playerID,
                    monsterID,
                    4),
                "Melee approach queues through the existing engagement seam");

            AdvanceTicks(world, 4);

            test.Expect(
                monster != nullptr &&
                    std::abs(player->GetPosition().GetX() - monster->GetPosition().GetX()) <= 1 &&
                    std::abs(player->GetPosition().GetY() - monster->GetPosition().GetY()) <= 1,
                "Player melee approach stops adjacent to its target");
            test.Expect(
                monster != nullptr &&
                    (player->GetPosition().GetX() != monster->GetPosition().GetX() ||
                     player->GetPosition().GetY() != monster->GetPosition().GetY()),
                "Player melee approach does not enter the target tile");

            const Action *action =
                world.GetActionForEntity(playerID);

            test.Expect(
                action != nullptr &&
                    action->GetType() == ActionType::MELEE_ATTACK &&
                    action->GetTargetID() == monsterID,
                "Successful melee approach still starts the requested engagement");
        }
    }

    void TestMonsterDynamicObstruction(TestContext &test)
    {
        {
            World world;
            int playerID = world.CreatePlayer();
            int monsterID = world.CreateMonster(
                40,
                40,
                CombatRatings{8, 8, 8, 40},
                RewardTableType::NONE,
                std::nullopt,
                MonsterAggressionDefinition{8, 12});
            Player *player = GetPlayer(world, playerID);
            Monster *monster = GetMonster(world, monsterID);

            player->GetPosition().SetPosition(45, 41);

            world.Update();
            world.Update();

            test.ExpectEqual(
                monster->GetPosition().GetX(),
                41,
                "Aggressive chase executes its first queued tile step");

            world.GetMap().SetTileType(
                42,
                40,
                TileType::WALL);

            const int obstructedX =
                monster->GetPosition().GetX();
            const int obstructedY =
                monster->GetPosition().GetY();

            world.Update();

            test.ExpectEqual(
                ManhattanDistance(
                    obstructedX,
                    obstructedY,
                    monster->GetPosition().GetX(),
                    monster->GetPosition().GetY()),
                0,
                "Aggressive chase does not move again while recalculating");

            world.Update();

            test.ExpectEqual(
                ManhattanDistance(
                    obstructedX,
                    obstructedY,
                    monster->GetPosition().GetX(),
                    monster->GetPosition().GetY()),
                1,
                "Aggressive chase takes one legal detour step after obstruction");
        }

        {
            World world;
            int playerID = world.CreatePlayer();
            int monsterID = world.CreateMonster(
                70,
                70,
                CombatRatings{8, 8, 8, 40},
                RewardTableType::NONE,
                std::nullopt,
                MonsterAggressionDefinition{5, 20});
            Player *player = GetPlayer(world, playerID);
            Monster *monster = GetMonster(world, monsterID);

            player->GetPosition().SetPosition(0, 0);
            monster->GetPosition().SetPosition(80, 70);

            world.Update();
            world.Update();

            test.ExpectEqual(
                monster->GetPosition().GetX(),
                79,
                "Monster return-home path executes its first tile step");

            world.GetMap().SetTileType(
                78,
                70,
                TileType::WALL);

            const int obstructedX =
                monster->GetPosition().GetX();
            const int obstructedY =
                monster->GetPosition().GetY();

            world.Update();

            test.ExpectEqual(
                ManhattanDistance(
                    obstructedX,
                    obstructedY,
                    monster->GetPosition().GetX(),
                    monster->GetPosition().GetY()),
                0,
                "Return-home movement does not move while recalculating");

            world.Update();

            test.ExpectEqual(
                ManhattanDistance(
                    obstructedX,
                    obstructedY,
                    monster->GetPosition().GetX(),
                    monster->GetPosition().GetY()),
                1,
                "Return-home movement takes one legal detour step after obstruction");
        }
    }

    void TestMovementActionCancellation(TestContext &test)
    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();
        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(80, 80);
        defender->GetPosition().SetPosition(81, 80);

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                3),
            "Action cancellation setup starts a melee action");

        world.QueueMovementRequest(
            MovementRequest(
                attackerID,
                -1,
                0));
        world.Update();

        test.ExpectEqual(
            attacker->GetPosition().GetX(),
            79,
            "Successful direct movement still executes after action cancellation");
        test.Expect(
            world.GetActionForEntity(attackerID) == nullptr,
            "Successful movement still cancels the incompatible action");
        test.Expect(
            ContainsCancellationReason(
                world.GetActionLifecycleEvents(),
                ActionCancelReason::PLAYER_MOVED),
            "Movement cancellation keeps the existing PLAYER_MOVED reason");
    }
}

int main()
{
    TestContext test;

    TestMovementPrimitive(test);
    TestActivePathProgress(test);
    TestSuccessfulDynamicRecalculation(test);
    TestFailedDynamicRecalculation(test);
    TestDestinationAndInteractionApproaches(test);
    TestMonsterDynamicObstruction(test);
    TestMovementActionCancellation(test);

    return test.Finish();
}
