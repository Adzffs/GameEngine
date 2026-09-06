#include "TestSupport.h"

#include "../src/Core/EngineWorldClickDecision.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Player/Player.h"

int main()
{
    TestContext test;

    {
        World world;
        const int playerID = world.CreatePlayer();
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        player->GetPosition().SetPosition(2, 2);
        EngineWorldClickDecision::Route(
            world, playerID, 3, 3, std::nullopt, 1);
        world.Update();
        test.Expect(world.GetActiveDialogueSession(playerID) != nullptr,
                    "Friendly NPC routing queues TALK");
        test.Expect(!world.HasPendingMeleeEngagement(playerID) &&
                        !world.HasActiveMovementPath(playerID),
                    "Friendly NPC routing queues neither attack nor movement when adjacent");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        const int monsterID = world.CreateMonster(
            7, 2, CombatRatings{8, 8, 8, 40});
        EngineWorldClickDecision::Route(
            world, playerID, 7, 2, monsterID, std::nullopt);
        world.Update();
        test.Expect(world.HasPendingMeleeEngagement(playerID),
                    "Monster routing retains attack behavior");
        test.Expect(world.GetActiveDialogueSession(playerID) == nullptr,
                    "Monster routing does not start dialogue");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        player->GetPosition().SetPosition(0, 0);
        ResourceNode *resource = world.GetResourceAt(5, 5);
        test.Expect(resource != nullptr, "Resource priority fixture exists");
        EngineWorldClickDecision::Route(
            world, playerID, 5, 5, 2, 1);
        world.Update();
        test.Expect(world.HasActiveMovementPath(playerID) &&
                        !world.HasPendingMeleeEngagement(playerID) &&
                        world.GetActiveDialogueSession(playerID) == nullptr,
                    "Resource routing retains precedence over entity hits");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        player->GetPosition().SetPosition(0, 0);
        test.Expect(world.GetStationAt(14, 9) != nullptr,
                    "Station priority fixture exists");
        EngineWorldClickDecision::Route(
            world, playerID, 14, 9, 2, 1);
        world.Update();
        test.Expect(world.HasActiveMovementPath(playerID) &&
                        !world.HasPendingMeleeEngagement(playerID) &&
                        world.GetActiveDialogueSession(playerID) == nullptr,
                    "Station routing retains precedence over entity hits");
    }

    {
        World world;
        const int playerID = world.CreatePlayer();
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        player->GetPosition().SetPosition(2, 2);
        EngineWorldClickDecision::Route(
            world, playerID, 8, 2, std::nullopt, std::nullopt);
        world.Update();
        test.Expect(world.HasActiveMovementPath(playerID),
                    "Empty-ground routing retains movement behavior");
    }

    return test.Finish();
}
