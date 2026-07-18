#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Action/ActionType.h"
#include "../src/Core/EngineClickRouting.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Graphics/Graphics.h"
#include "../src/Player/Player.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/Object/Station/StationType.h"
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
    Graphics graphics;

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

        SDL_FRect monsterRectangle =
            graphics.GetMonsterScreenRectangle(*monster);

        int startX = player->GetPosition().GetX();
        int startY = player->GetPosition().GetY();

        test.Expect(
            EngineClickRouting::HandleWorldClick(
                graphics,
                world,
                playerID,
                monster->GetPosition().GetX(),
                monster->GetPosition().GetY(),
                static_cast<int>(monsterRectangle.x + 1.0f),
                static_cast<int>(monsterRectangle.y + 1.0f)),
            "Monster click should be handled by the routing helper");

        test.Expect(
            world.HasPendingMeleeEngagement(playerID),
            "Monster click queues a pending melee engagement");
        test.ExpectEqual(
            player->GetPosition().GetX(),
            startX,
            "Monster click does not move the player immediately on X");
        test.ExpectEqual(
            player->GetPosition().GetY(),
            startY,
            "Monster click does not move the player immediately on Y");
        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Monster click does not start a combat action immediately");

        AdvanceWorldTicks(world, 6);

        const Action *action =
            world.GetActionForEntity(playerID);

        test.Expect(
            action != nullptr,
            "Monster click eventually starts a combat action when in range");

        if (action != nullptr)
        {
            test.ExpectEqual(
                static_cast<int>(action->GetType()),
                static_cast<int>(ActionType::MELEE_ATTACK),
                "Monster click starts a melee attack action");

            test.Expect(
                action->IsRepeating(),
                "Monster click starts a repeating melee attack");
        }
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            7,
            2,
            CombatRatings{8, 8, 8, 40});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(2, 2);

        SDL_FRect monsterRectangle =
            graphics.GetMonsterScreenRectangle(*monster);

        test.Expect(
            EngineClickRouting::HandleWorldClick(
                graphics,
                world,
                playerID,
                12,
                2,
                static_cast<int>(monsterRectangle.x + 1.0f),
                static_cast<int>(monsterRectangle.y + 1.0f)),
            "Second monster click should be handled");

        test.Expect(
            world.HasPendingMeleeEngagement(playerID),
            "First monster click creates a pending engagement");

        int firstTarget = monsterID;

        int secondMonsterID = world.CreateMonster(
            2,
            7,
            CombatRatings{8, 8, 8, 40});

        Monster *secondMonster = GetMonster(world, secondMonsterID);
        SDL_FRect secondRectangle =
            graphics.GetMonsterScreenRectangle(*secondMonster);

        test.Expect(
            EngineClickRouting::HandleWorldClick(
                graphics,
                world,
                playerID,
                2,
                7,
                static_cast<int>(secondRectangle.x + 1.0f),
                static_cast<int>(secondRectangle.y + 1.0f)),
            "A different monster click should also be handled");

        test.Expect(
            world.HasPendingMeleeEngagement(playerID),
            "A second monster click keeps the pending melee request active");

        AdvanceWorldTicks(world, 6);

        const Action *action =
            world.GetActionForEntity(playerID);

        test.Expect(
            action != nullptr,
            "The replacement monster click starts a combat action");

        if (action != nullptr)
        {
            test.ExpectEqual(
                action->GetTargetID(),
                secondMonsterID,
                "The later monster click replaces the previous target");
        }

        test.ExpectEqual(
            firstTarget,
            monsterID,
            "The first monster ID is preserved for comparison");
    }

    {
        World world;

        int playerID = world.CreatePlayer();

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(2, 2);

        test.Expect(
            EngineClickRouting::HandleWorldClick(
                graphics,
                world,
                playerID,
                8,
                2,
                260,
                80),
            "Ground click should be handled");

        test.Expect(
            !world.HasPendingMeleeEngagement(playerID),
            "Ground click does not queue a melee engagement");

        AdvanceWorldTicks(world, 1);

        test.Expect(
            player->GetPosition().GetX() != 2 ||
                player->GetPosition().GetY() != 2,
            "Ground click queues ordinary movement");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(13, 9);

        CraftingStation *station = world.GetStationAt(14, 9);
        test.Expect(
            station != nullptr,
            "Furnace exists for the station priority test");

        int monsterID = world.CreateMonster(
            14,
            9,
            CombatRatings{8, 8, 8, 40});

        Monster *monster = GetMonster(world, monsterID);
        SDL_FRect monsterRectangle =
            graphics.GetMonsterScreenRectangle(*monster);

        test.Expect(
            EngineClickRouting::HandleWorldClick(
                graphics,
                world,
                playerID,
                14,
                9,
                static_cast<int>(monsterRectangle.x + 1.0f),
                static_cast<int>(monsterRectangle.y + 1.0f)),
            "Station-tile click should be handled");

        test.Expect(
            !world.HasPendingMeleeEngagement(playerID),
            "Station click takes priority over Monster selection");

        AdvanceWorldTicks(world, 1);

        StationType openedStationType = StationType::NONE;

        test.Expect(
            world.ConsumeOpenedStation(
                playerID,
                openedStationType),
            "Station click opens the station menu path");

        test.ExpectEqual(
            static_cast<int>(openedStationType),
            static_cast<int>(StationType::FURNACE),
            "The furnace is the opened station");
    }

    {
        World world;

        int monsterID = world.CreateMonster(
            4,
            4,
            CombatRatings{8, 8, 8, 40});

        Monster *monster = GetMonster(world, monsterID);
        SDL_FRect monsterRectangle =
            graphics.GetMonsterScreenRectangle(*monster);

        monster->ApplyDamage(10000);

        std::optional<int> hit =
            graphics.GetMonsterAtScreenPosition(
                static_cast<int>(monsterRectangle.x + 1.0f),
                static_cast<int>(monsterRectangle.y + 1.0f),
                world.GetEntities());

        test.Expect(
            !hit.has_value(),
            "Dead monsters are ignored by hit-testing");
    }

    return test.Finish();
}