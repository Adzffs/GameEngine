#include "TestSupport.h"

#include <optional>

#include "../src/Entity/Monster/Monster.h"
#include "../src/Graphics/Graphics.h"
#include "../src/Player/Player.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/World.h"

namespace
{
    Monster *GetMonster(
        World &world,
        int entityID)
    {
        return dynamic_cast<Monster *>(
            world.GetEntityByID(entityID));
    }

    void ExpectRectangle(
        TestContext &test,
        const SDL_FRect &rectangle,
        float expectedX,
        float expectedY,
        float expectedW,
        float expectedH,
        const char *message)
    {
        test.ExpectNear(
            rectangle.x,
            expectedX,
            0.001f,
            std::string(message) + " x");
        test.ExpectNear(
            rectangle.y,
            expectedY,
            0.001f,
            std::string(message) + " y");
        test.ExpectNear(
            rectangle.w,
            expectedW,
            0.001f,
            std::string(message) + " w");
        test.ExpectNear(
            rectangle.h,
            expectedH,
            0.001f,
            std::string(message) + " h");
    }
}

int main()
{
    TestContext test;
    Graphics graphics;

    {
        World world;

        int monsterID = world.CreateMonster(
            7,
            4,
            CombatRatings{8, 8, 8, 40});

        Monster *monster = GetMonster(world, monsterID);

        SDL_FRect rectangle =
            graphics.GetMonsterScreenRectangle(*monster);

        ExpectRectangle(
            test,
            rectangle,
            229.0f,
            133.0f,
            22.0f,
            22.0f,
            "Living monster rectangle matches the expected tile inset");
    }

    {
        World world;

        int monsterID = world.CreateMonster(
            3,
            3,
            CombatRatings{8, 8, 8, 40});

        Monster *monster = GetMonster(world, monsterID);
        SDL_FRect rectangle =
            graphics.GetMonsterScreenRectangle(*monster);

        std::optional<int> hit =
            graphics.GetMonsterAtScreenPosition(
                static_cast<int>(rectangle.x + 1.0f),
                static_cast<int>(rectangle.y + 1.0f),
                world.GetEntities());

        test.Expect(
            hit.has_value(),
            "A point inside the monster rectangle should hit");

        if (hit.has_value())
        {
            test.ExpectEqual(
                hit.value(),
                monsterID,
                "The inside point returns the monster ID");
        }

        std::optional<int> miss =
            graphics.GetMonsterAtScreenPosition(
                static_cast<int>(rectangle.x + rectangle.w + 1.0f),
                static_cast<int>(rectangle.y + rectangle.h + 1.0f),
                world.GetEntities());

        test.Expect(
            !miss.has_value(),
            "A point outside the monster rectangle should miss");
    }

    {
        World world;

        int firstMonsterID = world.CreateMonster(
            5,
            5,
            CombatRatings{8, 8, 8, 40});
        int secondMonsterID = world.CreateMonster(
            5,
            5,
            CombatRatings{8, 8, 8, 40});

        std::optional<int> hit =
            graphics.GetMonsterAtScreenPosition(
                165,
                165,
                world.GetEntities());

        test.Expect(
            hit.has_value(),
            "Overlapping monsters should still produce a hit");

        if (hit.has_value())
        {
            test.ExpectEqual(
                hit.value(),
                firstMonsterID,
                "Hit-testing returns the first living monster in stable entity order");
        }

        test.Expect(
            firstMonsterID < secondMonsterID,
            "Monster IDs increase in creation order for the overlap policy test");
    }

    {
        World world;

        int monsterID = world.CreateMonster(
            9,
            7,
            CombatRatings{8, 8, 8, 40});

        Monster *monster = GetMonster(world, monsterID);
        SDL_FRect rectangle =
            graphics.GetMonsterScreenRectangle(
                *monster,
                2,
                1);

        ExpectRectangle(
            test,
            rectangle,
            229.0f,
            197.0f,
            22.0f,
            22.0f,
            "Camera offset shifts the monster rectangle");

        std::optional<int> hit =
            graphics.GetMonsterAtScreenPosition(
                230,
                198,
                world.GetEntities(),
                2,
                1);

        test.Expect(
            hit.has_value() && hit.value() == monsterID,
            "Camera offset is respected during hit-testing");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            4,
            4,
            CombatRatings{9, 10, 11, 40});

        Player *player = dynamic_cast<Player *>(
            world.GetEntityByID(playerID));
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(1, 1);

        int startHealth = monster->GetCurrentHealth();
        auto startPosition = monster->GetPosition();
        CombatRatings startRatings = monster->GetCombatRatings();
        SDL_FRect rectangle =
            graphics.GetMonsterScreenRectangle(*monster);

        std::optional<int> hit =
            graphics.GetMonsterAtScreenPosition(
                static_cast<int>(rectangle.x + 1.0f),
                static_cast<int>(rectangle.y + 1.0f),
                world.GetEntities());

        test.Expect(
            hit.has_value(),
            "Read-only hit-testing should still see the monster");

        test.ExpectEqual(
            monster->GetCurrentHealth(),
            startHealth,
            "Hit-testing does not change monster health");
        test.ExpectEqual(
            monster->GetPosition().GetX(),
            startPosition.GetX(),
            "Hit-testing does not change monster X position");
        test.ExpectEqual(
            monster->GetPosition().GetY(),
            startPosition.GetY(),
            "Hit-testing does not change monster Y position");
        test.Expect(
            monster->GetCombatRatings().attackAccuracy == startRatings.attackAccuracy &&
                monster->GetCombatRatings().meleeStrength == startRatings.meleeStrength &&
                monster->GetCombatRatings().defence == startRatings.defence &&
                monster->GetCombatRatings().maximumHealth == startRatings.maximumHealth,
            "Hit-testing does not change monster combat ratings");
        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Hit-testing does not create a player action");
        test.Expect(
            world.GetActionForEntity(monsterID) == nullptr,
            "Hit-testing does not create a monster action");
    }

    {
        World world;

        int monsterID = world.CreateMonster(
            6,
            6,
            CombatRatings{8, 8, 8, 40});

        Monster *monster = GetMonster(world, monsterID);
        SDL_FRect rectangle =
            graphics.GetMonsterScreenRectangle(*monster);

        monster->ApplyDamage(10000);

        std::optional<int> hit =
            graphics.GetMonsterAtScreenPosition(
                static_cast<int>(rectangle.x + 1.0f),
                static_cast<int>(rectangle.y + 1.0f),
                world.GetEntities());

        test.Expect(
            !hit.has_value(),
            "Dead monsters are not selectable");
    }

    {
        World world;

        std::optional<int> hit =
            graphics.GetMonsterAtScreenPosition(
                -10,
                -10,
                world.GetEntities());

        test.Expect(
            !hit.has_value(),
            "Out-of-map coordinates safely return no result");
    }

    return test.Finish();
}