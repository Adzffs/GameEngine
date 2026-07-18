#include "TestSupport.h"

#include "../src/Action/ActionType.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Player/Player.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/World.h"

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

    void AdvanceWorldTicks(
        World &world,
        int ticks)
    {
        for (int index = 0; index < ticks; ++index)
        {
            world.Update();
        }
    }
}

int main()
{
    TestContext test;

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            5,
            2,
            CombatRatings{8, 8, 8, 40});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(2, 2);
        monster->GetPosition().SetPosition(5, 2);

        test.Expect(
            world.QueueMeleeEngagementRequest(
                playerID,
                monsterID,
                4),
            "Queueing a melee request should succeed");

        test.Expect(
            world.HasPendingMeleeEngagement(playerID),
            "Queued melee request is tracked as pending");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            6,
            2,
            CombatRatings{8, 8, 8, 40});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(2, 2);
        monster->GetPosition().SetPosition(6, 2);

        test.Expect(
            world.QueueMeleeEngagementRequest(
                playerID,
                monsterID,
                3),
            "Pending melee request should queue before movement");

        AdvanceWorldTicks(world, 6);

        test.Expect(
            !world.HasPendingMeleeEngagement(playerID),
            "Pending melee request clears after the attack starts");

        const Action *action =
            world.GetActionForEntity(playerID);

        test.Expect(
            action != nullptr,
            "A melee action starts after the player reaches the target");

        if (action != nullptr)
        {
            test.ExpectEqual(
                static_cast<int>(action->GetType()),
                static_cast<int>(ActionType::MELEE_ATTACK),
                "Started action type is melee attack");

            test.Expect(
                action->IsRepeating(),
                "Started melee action repeats");
        }
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int firstMonsterID = world.CreateMonster(
            5,
            2,
            CombatRatings{8, 8, 8, 40});
        int secondMonsterID = world.CreateMonster(
            2,
            5,
            CombatRatings{8, 8, 8, 40});

        Player *player = GetPlayer(world, playerID);
        Monster *firstMonster = GetMonster(world, firstMonsterID);
        Monster *secondMonster = GetMonster(world, secondMonsterID);

        player->GetPosition().SetPosition(2, 2);
        firstMonster->GetPosition().SetPosition(5, 2);
        secondMonster->GetPosition().SetPosition(2, 5);

        test.Expect(
            world.QueueMeleeEngagementRequest(
                playerID,
                firstMonsterID,
                3),
            "First melee request queues successfully");

        test.Expect(
            world.QueueMeleeEngagementRequest(
                playerID,
                secondMonsterID,
                3),
            "Second melee request replaces the first one");

        AdvanceWorldTicks(world, 6);

        const Action *action =
            world.GetActionForEntity(playerID);

        test.Expect(
            action != nullptr,
            "Replacement request still starts a melee action");

        if (action != nullptr)
        {
            test.ExpectEqual(
                action->GetTargetID(),
                secondMonsterID,
                "Replacement melee request targets the newer defender");
        }
    }

    return test.Finish();
}