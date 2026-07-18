#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Action/ActionCancelReason.h"
#include "../src/Combat/MeleeCombatFeedback.h"
#include "../src/Core/RandomSource.h"
#include "../src/Entity/EntityType.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Graphics/Graphics.h"
#include "../src/Player/Player.h"
#include "../src/Skills/SkillType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/World.h"

#include <optional>
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

    void BoostAttackSkill(Player &player)
    {
        player.GetSkills().AddXP(
            SkillType::ATTACK,
            100000);
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

    CombatRatings MakeRatings(
        int attackAccuracy,
        int meleeStrength,
        int defence,
        int maximumHealth)
    {
        CombatRatings ratings;
        ratings.attackAccuracy = attackAccuracy;
        ratings.meleeStrength = meleeStrength;
        ratings.defence = defence;
        ratings.maximumHealth = maximumHealth;
        return ratings;
    }
}

int main()
{
    TestContext test;
    Graphics graphics;

    {
        test.ExpectEqual(
            graphics.CalculateHealthRatio(10, 10),
            1.0f,
            "Health ratio is 1 at full health");
        test.ExpectNear(
            graphics.CalculateHealthRatio(5, 10),
            0.5f,
            0.001f,
            "Health ratio is half at half health");
        test.ExpectEqual(
            graphics.CalculateHealthRatio(0, 10),
            0.0f,
            "Health ratio is zero at zero health");
        test.ExpectEqual(
            graphics.CalculateHealthRatio(5, 0),
            0.0f,
            "Health ratio clamps safely when maximum health is zero");
        test.ExpectEqual(
            graphics.CalculateHealthRatio(12, 10),
            1.0f,
            "Health ratio clamps above one");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 2, 2}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            6,
            2,
            MakeRatings(8, 8, 6, 30));

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        BoostAttackSkill(*player);
        player->GetPosition().SetPosition(5, 2);

        test.Expect(
            world.TryStartMeleeAttack(
                playerID,
                monsterID,
                2),
            "Starting a melee attack succeeds");

        test.Expect(
            world.GetMeleeCombatFeedbacks().empty(),
            "Starting a melee attack does not immediately create feedback");

        world.Update();

        test.Expect(
            world.GetMeleeCombatFeedbacks().empty(),
            "Feedback is not created before the attack completes");

        world.Update();

        const auto &feedbacks = world.GetMeleeCombatFeedbacks();

        test.Expect(
            feedbacks.count(monsterID) == 1,
            "Resolved hit creates feedback for the defender");

        if (feedbacks.count(monsterID) == 1)
        {
            const MeleeCombatFeedback &feedback = feedbacks.at(monsterID);

            test.Expect(
                feedback.hit,
                "Resolved hit feedback is marked as a hit");
            test.ExpectEqual(
                feedback.actualDamageApplied,
                world.GetLastMeleeAttackResult()->actualDamageApplied,
                "Feedback uses actualDamageApplied from the last melee result");
            test.ExpectEqual(
                feedback.defenderEntityID,
                monsterID,
                "Feedback identifies the correct defender");
            test.ExpectEqual(
                feedback.remainingTicks,
                3,
                "Fresh feedback starts with the full lifetime");
        }

        SDL_FRect monsterRectangle =
            graphics.GetMonsterScreenRectangle(*monster);

        SDL_FPoint feedbackPosition =
            graphics.GetMonsterCombatFeedbackPosition(*monster);

        test.Expect(
            feedbackPosition.x >= monsterRectangle.x,
            "Feedback position is based on the monster rectangle X");
        test.Expect(
            feedbackPosition.y < monsterRectangle.y,
            "Feedback position is placed above the monster rectangle");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 2, 2}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            8,
            2,
            MakeRatings(8, 8, 6, 30));

        Player *player = GetPlayer(world, playerID);
        BoostAttackSkill(*player);
        player->GetPosition().SetPosition(2, 2);

        test.Expect(
            world.QueueMeleeEngagementRequest(
                playerID,
                monsterID,
                2),
            "Walking engagement starts from range");

        world.Update();

        test.Expect(
            world.GetMeleeCombatFeedbacks().empty(),
            "No feedback is created while the attacker is still walking");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{3, 4}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            6,
            2,
            MakeRatings(8, 8, 6, 30));

        Player *player = GetPlayer(world, playerID);
        BoostAttackSkill(*player);
        player->GetPosition().SetPosition(5, 2);

        test.Expect(
            world.TryStartMeleeAttack(
                playerID,
                monsterID,
                1),
            "Miss test melee attack starts");

        world.Update();

        const auto &feedbacks = world.GetMeleeCombatFeedbacks();

        test.Expect(
            feedbacks.count(monsterID) == 1,
            "Miss creates feedback for the defender");

        if (feedbacks.count(monsterID) == 1)
        {
            const MeleeCombatFeedback &feedback = feedbacks.at(monsterID);

            test.Expect(
                !feedback.hit,
                "Miss feedback is marked as a miss");
            test.ExpectEqual(
                feedback.actualDamageApplied,
                0,
                "Miss feedback shows zero actual damage");
        }
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 1, 0}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            6,
            2,
            MakeRatings(8, 8, 6, 30));

        Player *player = GetPlayer(world, playerID);
        BoostAttackSkill(*player);
        player->GetPosition().SetPosition(5, 2);

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        const auto &feedbacks = world.GetMeleeCombatFeedbacks();

        test.Expect(
            feedbacks.count(monsterID) == 1,
            "Zero-damage hit creates feedback");

        if (feedbacks.count(monsterID) == 1)
        {
            const MeleeCombatFeedback &feedback = feedbacks.at(monsterID);

            test.Expect(
                feedback.hit,
                "Zero-damage hit is still marked as a hit");
            test.ExpectEqual(
                feedback.actualDamageApplied,
                0,
                "Zero-damage hit remains distinguishable from a miss");
        }
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 2, 2}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            6,
            2,
            MakeRatings(8, 8, 6, 30));

        Player *player = GetPlayer(world, playerID);
        BoostAttackSkill(*player);
        player->GetPosition().SetPosition(5, 2);

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            2);

        world.CancelActionsForEntity(
            playerID,
            ActionCancelReason::INTERFACE_CLOSED);

        world.Update();

        test.Expect(
            world.GetMeleeCombatFeedbacks().empty(),
            "Cancelled attack creates no feedback");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 2, 2}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            8,
            2,
            MakeRatings(8, 8, 6, 30));

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        BoostAttackSkill(*player);
        player->GetPosition().SetPosition(5, 2);

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        monster->GetPosition().SetPosition(12, 12);

        world.Update();
        world.Update();

        test.Expect(
            world.GetMeleeCombatFeedbacks().empty(),
            "Invalidated completion creates no feedback");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 2, 2}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            6,
            2,
            MakeRatings(8, 8, 6, 2));

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        BoostAttackSkill(*player);
        player->GetPosition().SetPosition(5, 2);

        test.Expect(
            world.TryStartMeleeEngagement(
                playerID,
                monsterID,
                1),
            "Repeating melee engagement starts");

        world.Update();
        world.Update();

        test.Expect(
            !monster->IsAlive(),
            "Monster dies from the resolved repeated engagement hit");
        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Repeating combat stops when the defender dies");
        test.Expect(
            world.GetEntityByID(monsterID) != nullptr,
            "Dead monster remains in EntityManager");

        SDL_FRect monsterRectangle = graphics.GetMonsterScreenRectangle(*monster);
        std::optional<int> hit = graphics.GetMonsterAtScreenPosition(
            static_cast<int>(monsterRectangle.x + 1.0f),
            static_cast<int>(monsterRectangle.y + 1.0f),
            world.GetEntities());

        test.Expect(
            !hit.has_value(),
            "Dead monster is not returned by hit-testing");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 2, 2}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            6,
            2,
            MakeRatings(8, 8, 6, 30));

        Player *player = GetPlayer(world, playerID);
        BoostAttackSkill(*player);
        player->GetPosition().SetPosition(5, 2);

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();
        world.Update();

        test.Expect(
            !world.GetMeleeCombatFeedbacks().empty(),
            "Feedback exists immediately after a resolved hit");

        AdvanceWorldTicks(world, 3);

        test.Expect(
            world.GetMeleeCombatFeedbacks().empty(),
            "Feedback expires after the configured tick lifetime");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 2, 2, 6, 2, 2}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            6,
            2,
            MakeRatings(8, 8, 6, 30));

        Player *player = GetPlayer(world, playerID);
        BoostAttackSkill(*player);
        player->GetPosition().SetPosition(5, 2);

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        const auto &firstFeedbacks = world.GetMeleeCombatFeedbacks();
        test.Expect(
            firstFeedbacks.count(monsterID) == 1,
            "First attack creates feedback for the defender");

        if (firstFeedbacks.count(monsterID) == 1)
        {
            test.ExpectEqual(
                firstFeedbacks.at(monsterID).remainingTicks,
                3,
                "Fresh feedback starts with the full lifetime");
        }

        world.Update();

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        const auto &secondFeedbacks = world.GetMeleeCombatFeedbacks();

        test.Expect(
            secondFeedbacks.count(monsterID) == 1,
            "Repeated attack keeps a single defender feedback entry");

        if (secondFeedbacks.count(monsterID) == 1)
        {
            test.ExpectEqual(
                secondFeedbacks.at(monsterID).remainingTicks,
                3,
                "Repeated attack refreshes the defender feedback lifetime");
        }
    }

    return test.Finish();
}