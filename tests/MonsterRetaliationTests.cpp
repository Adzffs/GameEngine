#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Action/ActionType.h"
#include "../src/Combat/MeleeCombatFeedback.h"
#include "../src/Core/RandomSource.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Graphics/Graphics.h"
#include "../src/Player/Player.h"
#include "../src/Recipe/RecipeType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/World.h"

#include <memory>
#include <utility>
#include <vector>

namespace
{
    class SequenceRandomSource : public RandomSource
    {
    public:
        explicit SequenceRandomSource(std::vector<int> values)
            : values(std::move(values))
        {
        }

        int NextIntInclusive(
            int minimum,
            int maximum) override
        {
            (void)minimum;
            (void)maximum;

            if (nextIndex >= static_cast<int>(values.size()))
            {
                return 0;
            }

            return values[nextIndex++];
        }

    private:
        std::vector<int> values;
        int nextIndex = 0;
    };

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
            9,
            3,
            CombatRatings{10, 9, 8, 60});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(8, 3);
        monster->GetPosition().SetPosition(9, 3);

        AdvanceWorldTicks(world, 8);

        test.Expect(
            world.GetActionForEntity(monsterID) == nullptr,
            "Monster remains passive before it is attacked");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            10,
            2,
            CombatRatings{8, 8, 8, 60});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(2, 2);
        monster->GetPosition().SetPosition(10, 2);

        test.Expect(
            world.QueueMeleeEngagementRequest(
                playerID,
                monsterID,
                4),
            "Walking melee request is accepted before adjacency");

        world.Update();

        test.Expect(
            world.GetActionForEntity(monsterID) == nullptr,
            "Monster retaliation does not start while the player is still walking");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            9,
            6,
            CombatRatings{10, 10, 8, 80});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(8, 6);
        monster->GetPosition().SetPosition(9, 6);

        test.Expect(
            world.TryStartMeleeEngagement(
                playerID,
                monsterID,
                4),
            "Player engagement starts when adjacent");

        const Action *playerAction =
            world.GetActionForEntity(playerID);

        const Action *monsterAction =
            world.GetActionForEntity(monsterID);

        test.Expect(
            playerAction != nullptr,
            "Player receives an active melee engagement action");
        test.Expect(
            monsterAction != nullptr,
            "Successful player engagement starts Monster retaliation");

        if (playerAction != nullptr)
        {
            test.ExpectEqual(
                playerAction->GetOwnerID(),
                playerID,
                "Player action owner is the player");
            test.ExpectEqual(
                playerAction->GetTargetID(),
                monsterID,
                "Player action targets the monster");
            test.Expect(
                playerAction->IsRepeating(),
                "Player engagement remains repeating");
        }

        if (monsterAction != nullptr)
        {
            test.ExpectEqual(
                monsterAction->GetOwnerID(),
                monsterID,
                "Monster retaliation owner is the monster");
            test.ExpectEqual(
                monsterAction->GetTargetID(),
                playerID,
                "Monster retaliation targets the correct player");
            test.Expect(
                monsterAction->IsRepeating(),
                "Monster retaliation is repeating");
            test.ExpectEqual(
                monsterAction->GetDuration(),
                5,
                "Monster retaliation uses the fixed temporary duration");
        }
    }

    {
        World world;

        int playerOneID = world.CreatePlayer();
        int playerTwoID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            CombatRatings{10, 10, 8, 80});

        Player *playerOne = GetPlayer(world, playerOneID);
        Player *playerTwo = GetPlayer(world, playerTwoID);
        Monster *monster = GetMonster(world, monsterID);

        playerOne->GetPosition().SetPosition(10, 10);
        playerTwo->GetPosition().SetPosition(12, 10);
        monster->GetPosition().SetPosition(11, 10);

        test.Expect(
            world.TryStartMeleeEngagement(
                monsterID,
                playerTwoID,
                7),
            "Monster starts an existing unrelated action for replacement validation");

        test.Expect(
            world.TryStartMeleeEngagement(
                playerOneID,
                monsterID,
                4),
            "Player engagement still starts while monster is already busy");

        const Action *monsterAction =
            world.GetActionForEntity(monsterID);

        test.Expect(
            monsterAction != nullptr,
            "Monster keeps its existing action");

        if (monsterAction != nullptr)
        {
            test.ExpectEqual(
                monsterAction->GetTargetID(),
                playerTwoID,
                "Existing monster action target is not replaced by retaliation");
            test.ExpectEqual(
                monsterAction->GetDuration(),
                7,
                "Existing monster action duration remains unchanged");
        }
    }

    {
        World world;

        int firstPlayerID = world.CreatePlayer();
        int secondPlayerID = world.CreatePlayer();

        Player *firstPlayer = GetPlayer(world, firstPlayerID);
        Player *secondPlayer = GetPlayer(world, secondPlayerID);

        firstPlayer->GetPosition().SetPosition(15, 10);
        secondPlayer->GetPosition().SetPosition(16, 10);

        test.Expect(
            world.TryStartMeleeEngagement(
                firstPlayerID,
                secondPlayerID,
                4),
            "Player versus player engagement starts");

        int monsterActionCount = 0;

        for (const auto &entity : world.GetEntities())
        {
            Monster *monster = dynamic_cast<Monster *>(entity.get());

            if (monster == nullptr)
            {
                continue;
            }

            if (world.GetActionForEntity(monster->GetID()) != nullptr)
            {
                monsterActionCount++;
            }
        }

        test.ExpectEqual(
            monsterActionCount,
            0,
            "Player versus player engagement does not create monster retaliation");
    }

    {
        World world;

        int firstMonsterID = world.CreateMonster(
            20,
            10,
            CombatRatings{9, 9, 9, 70});
        int secondMonsterID = world.CreateMonster(
            21,
            10,
            CombatRatings{9, 9, 9, 70});

        test.Expect(
            world.TryStartMeleeEngagement(
                firstMonsterID,
                secondMonsterID,
                4),
            "Monster versus monster engagement starts");

        test.Expect(
            world.GetActionForEntity(secondMonsterID) == nullptr,
            "Monster versus monster engagement does not create automatic retaliation");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{999, 0, 999}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            26,
            5,
            CombatRatings{60, 60, 10, 120});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(25, 5);
        monster->GetPosition().SetPosition(26, 5);

        int startingHealth = player->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeEngagement(
                playerID,
                monsterID,
                20),
            "Long player duration engagement starts for monster timing validation");

        AdvanceWorldTicks(world, 4);

        test.ExpectEqual(
            player->GetCurrentHealth(),
            startingHealth,
            "Monster retaliation does not deal immediate damage before completion interval");

        world.Update();

        test.Expect(
            player->GetCurrentHealth() < startingHealth,
            "Monster retaliation applies damage on its completion interval");
        test.Expect(
            world.GetMeleeCombatFeedbacks().count(playerID) == 1,
            "Monster retaliation creates combat feedback for player defenders");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            31,
            9,
            CombatRatings{8, 8, 8, 400});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(30, 9);
        monster->GetPosition().SetPosition(31, 9);

        test.Expect(
            world.TryStartMeleeEngagement(
                playerID,
                monsterID,
                2),
            "Engagement starts for independent timing validation");

        AdvanceWorldTicks(world, 2);

        const Action *playerAction =
            world.GetActionForEntity(playerID);

        const Action *monsterAction =
            world.GetActionForEntity(monsterID);

        test.Expect(
            playerAction != nullptr &&
                monsterAction != nullptr,
            "Both combatants retain independent active actions");

        if (playerAction != nullptr &&
            monsterAction != nullptr)
        {
            test.ExpectEqual(
                playerAction->GetDuration(),
                2,
                "Player action keeps player timing");
            test.ExpectEqual(
                monsterAction->GetDuration(),
                5,
                "Monster action keeps monster timing");
            test.Expect(
                monsterAction->GetProgress() > 0.0f &&
                    monsterAction->GetProgress() < 1.0f,
                "Monster action progress advances independently while player cycles faster");
        }
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{0, 999}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            36,
            7,
            CombatRatings{40, 40, 10, 300});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(35, 7);
        monster->GetPosition().SetPosition(36, 7);

        test.Expect(
            world.TryStartMeleeEngagement(
                playerID,
                monsterID,
                20),
            "Engagement starts for miss retaliation validation");

        AdvanceWorldTicks(world, 5);

        test.Expect(
            world.GetActionForEntity(monsterID) != nullptr,
            "Missed retaliation does not stop repeating monster engagement");

        if (world.GetMeleeCombatFeedbacks().count(playerID) == 1)
        {
            const MeleeCombatFeedback &feedback =
                world.GetMeleeCombatFeedbacks().at(playerID);

            test.Expect(
                !feedback.hit,
                "Missed retaliation records MISS feedback for player defenders");
        }
        else
        {
            test.Expect(
                false,
                "Missed retaliation should create feedback for the player defender");
        }
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{999, 0, 0}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            41,
            11,
            CombatRatings{40, 40, 10, 300});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(40, 11);
        monster->GetPosition().SetPosition(41, 11);

        test.Expect(
            world.TryStartMeleeEngagement(
                playerID,
                monsterID,
                20),
            "Engagement starts for zero-damage retaliation validation");

        AdvanceWorldTicks(world, 5);

        test.Expect(
            world.GetActionForEntity(monsterID) != nullptr,
            "Zero-damage retaliation hit does not stop repeating monster engagement");

        if (world.GetMeleeCombatFeedbacks().count(playerID) == 1)
        {
            const MeleeCombatFeedback &feedback =
                world.GetMeleeCombatFeedbacks().at(playerID);

            test.Expect(
                feedback.hit,
                "Zero-damage retaliation remains a hit event");
            test.ExpectEqual(
                feedback.actualDamageApplied,
                0,
                "Zero-damage retaliation feedback preserves zero applied damage");
        }
        else
        {
            test.Expect(
                false,
                "Zero-damage retaliation should create feedback for the player defender");
        }
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            46,
            8,
            CombatRatings{8, 8, 8, 90});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(45, 8);
        monster->GetPosition().SetPosition(46, 8);

        test.Expect(
            world.TryStartMeleeEngagement(
                playerID,
                monsterID,
                4),
            "Engagement starts before monster death cleanup");

        monster->ApplyDamage(10000);
        world.Update();

        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Monster death cancels the player engagement");
        test.Expect(
            world.GetActionForEntity(monsterID) == nullptr,
            "Monster death cancels the monster retaliation action");
        test.Expect(
            !world.HasPendingMeleeEngagement(playerID),
            "Monster death clears pending melee interactions targeting that combatant");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            51,
            4,
            CombatRatings{8, 8, 8, 90});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(50, 4);
        monster->GetPosition().SetPosition(51, 4);

        test.Expect(
            world.TryStartMeleeEngagement(
                playerID,
                monsterID,
                4),
            "Engagement starts before player death cleanup");

        player->ApplyDamage(10000);
        world.Update();

        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Player death cancels the player action");
        test.Expect(
            world.GetActionForEntity(monsterID) == nullptr,
            "Player death cancels monster retaliation targeting the player");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{999, 0, 999}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            56,
            6,
            CombatRatings{60, 60, 10, 100});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(55, 6);
        monster->GetPosition().SetPosition(56, 6);

        test.Expect(
            world.TryStartMeleeEngagement(
                playerID,
                monsterID,
                20),
            "Engagement starts before delayed-hit cleanup validation");

        world.Update();

        int healthBeforeDeath =
            player->GetCurrentHealth();

        monster->ApplyDamage(10000);

        AdvanceWorldTicks(world, 8);

        test.ExpectEqual(
            player->GetCurrentHealth(),
            healthBeforeDeath,
            "No delayed monster hit resolves after monster death");
        test.Expect(
            world.GetMeleeCombatFeedbacks().count(playerID) == 0,
            "No player feedback is created after monster death cancels retaliation");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            61,
            9,
            CombatRatings{8, 8, 8, 100});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(60, 9);
        monster->GetPosition().SetPosition(61, 9);

        world.TryStartMeleeEngagement(
            playerID,
            monsterID,
            4);

        player->ApplyDamage(10000);

        int deadX = player->GetPosition().GetX();
        int deadY = player->GetPosition().GetY();

        world.QueueMovementDestination(
            MovementDestinationRequest(
                playerID,
                deadX + 4,
                deadY));

        ResourceNode *resource =
            world.GetResourceAt(5, 5);

        if (resource != nullptr)
        {
            world.QueueResourceInteraction(
                playerID,
                resource->GetID());
        }

        CraftingStation *station =
            world.GetStationAt(14, 9);

        if (station != nullptr)
        {
            world.QueueStationInteraction(
                playerID,
                station->GetID());
        }

        world.QueueMeleeEngagementRequest(
            playerID,
            monsterID,
            4);

        world.Update();

        test.ExpectEqual(
            player->GetPosition().GetX(),
            deadX,
            "Dead player cannot move from ground clicks or destination requests");
        test.ExpectEqual(
            player->GetPosition().GetY(),
            deadY,
            "Dead player position remains unchanged");

        test.Expect(
            !world.TryStartMeleeEngagement(
                playerID,
                monsterID,
                4),
            "Dead player cannot start melee engagement actions");

        player->GetPosition().SetPosition(13, 9);

        test.Expect(
            !world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Dead player cannot start smithing actions");

        if (resource != nullptr)
        {
            world.QueueResourceInteraction(
                playerID,
                resource->GetID());
        }

        world.Update();

        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Dead player cannot start gathering actions");
        test.Expect(
            !world.HasPendingMeleeEngagement(playerID),
            "Pending melee requests owned by dead players are cleared");
    }

    {
        World world;

        int doomedPlayerID = world.CreatePlayer();
        int activePlayerID = world.CreatePlayer();
        int unrelatedMonsterID = world.CreateMonster(
            5,
            5,
            CombatRatings{8, 8, 8, 120});

        Player *doomedPlayer = GetPlayer(world, doomedPlayerID);
        Player *activePlayer = GetPlayer(world, activePlayerID);
        Monster *unrelatedMonster = GetMonster(world, unrelatedMonsterID);

        doomedPlayer->GetPosition().SetPosition(3, 4);
        activePlayer->GetPosition().SetPosition(4, 5);
        unrelatedMonster->GetPosition().SetPosition(5, 5);

        test.Expect(
            world.TryStartMeleeEngagement(
                activePlayerID,
                unrelatedMonsterID,
                4),
            "Unrelated player starts an action before death cleanup validation");

        const Action *activeActionBeforeDeath =
            world.GetActionForEntity(activePlayerID);

        test.Expect(
            activeActionBeforeDeath != nullptr &&
                activeActionBeforeDeath->GetType() == ActionType::MELEE_ATTACK,
            "Unrelated player action exists before death cleanup validation");

        doomedPlayer->ApplyDamage(10000);
        world.Update();

        const Action *activeActionAfterDeath =
            world.GetActionForEntity(activePlayerID);

        test.Expect(
            activeActionAfterDeath != nullptr &&
                activeActionAfterDeath->GetType() == ActionType::MELEE_ATTACK,
            "Unrelated entity actions remain active during dead-player cleanup");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            66,
            6,
            CombatRatings{8, 8, 8, 40});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(65, 6);
        monster->GetPosition().SetPosition(66, 6);

        player->ApplyDamage(10000);
        monster->ApplyDamage(10000);
        world.Update();

        test.Expect(
            world.GetEntityByID(playerID) != nullptr,
            "Dead player remains in EntityManager");
        test.Expect(
            world.GetEntityByID(monsterID) != nullptr,
            "Dead monster remains in EntityManager");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{999, 0, 999}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            71,
            3,
            CombatRatings{60, 60, 10, 120});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(70, 3);
        monster->GetPosition().SetPosition(71, 3);

        player->ApplyDamage(player->GetCurrentHealth() / 2);

        SDL_FRect playerBackground =
            graphics.GetPlayerHealthBarBackgroundRectangle(*player);

        SDL_FRect playerFill =
            graphics.GetPlayerHealthBarFillRectangle(*player);

        test.ExpectNear(
            playerFill.w,
            playerBackground.w * graphics.CalculateHealthRatio(
                                     player->GetCurrentHealth(),
                                     player->GetMaximumHealth()),
            0.001f,
            "Player health ratio helpers reuse the existing ratio calculation");

        test.Expect(
            world.TryStartMeleeEngagement(
                playerID,
                monsterID,
                20),
            "Engagement starts for player feedback position validation");

        AdvanceWorldTicks(world, 5);

        test.Expect(
            world.GetMeleeCombatFeedbacks().count(playerID) == 1,
            "Monster retaliation creates incoming feedback at the player defender");

        SDL_FRect playerRectangle =
            graphics.GetPlayerScreenRectangle(*player);

        SDL_FPoint playerFeedback =
            graphics.GetPlayerCombatFeedbackPosition(*player);

        test.Expect(
            playerFeedback.x >= playerRectangle.x,
            "Player feedback uses player screen X position");
        test.Expect(
            playerFeedback.y < playerRectangle.y,
            "Player feedback renders above the player rectangle");

        player->ApplyDamage(10000);

        SDL_FRect deadFill =
            graphics.GetPlayerHealthBarFillRectangle(*player);

        test.ExpectEqual(
            deadFill.w,
            0.0f,
            "Dead player health bar visual state is empty");
    }

    return test.Finish();
}
