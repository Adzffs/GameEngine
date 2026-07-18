#include "TestSupport.h"

#include "../src/Core/RandomSource.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Player/Player.h"
#include "../src/Reward/RewardTableType.h"
#include "../src/Skills/SkillType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/Event/EntityDiedEvent.h"
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
            if (minimum > maximum)
            {
                int temporary = minimum;
                minimum = maximum;
                maximum = temporary;
            }

            if (minimum == maximum)
            {
                return minimum;
            }

            if (nextIndex >= static_cast<int>(values.size()))
            {
                return minimum;
            }

            int value = values[nextIndex];
            nextIndex++;

            if (value < minimum)
            {
                return minimum;
            }

            if (value > maximum)
            {
                return maximum;
            }

            return value;
        }

    private:
        std::vector<int> values;
        int nextIndex = 0;
    };

    class InvalidRandomSource : public RandomSource
    {
    public:
        int NextIntInclusive(
            int minimum,
            int maximum) override
        {
            (void)minimum;
            (void)maximum;
            return 0;
        }
    };

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

    int GetWeightedOreCount(
        const Inventory &inventory)
    {
        return inventory.GetItemAmount(ItemType::COPPER_ORE) +
               inventory.GetItemAmount(ItemType::TIN_ORE) +
               inventory.GetItemAmount(ItemType::IRON_ORE);
    }

    void AdvanceTicks(
        World &world,
        int count)
    {
        for (int index = 0; index < count; ++index)
        {
            world.Update();
        }
    }

    void BoostAttackSkill(
        Player &player)
    {
        player.GetSkills().AddXP(
            SkillType::ATTACK,
            100000);
    }
}

int main()
{
    TestContext test;

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{9, 1, 1}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 20),
            RewardTableType::DEVELOPMENT_MONSTER);

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(10, 10);

        int coalBefore =
            player->GetInventory().GetItemAmount(ItemType::COAL);

        test.Expect(
            world.TryStartMeleeAttack(
                playerID,
                monsterID,
                1),
            "Nonlethal attack setup succeeds");

        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COAL),
            coalBefore,
            "Nonlethal attack grants no rewards");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{3, 4}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(10, 10);

        int coalBefore =
            player->GetInventory().GetItemAmount(ItemType::COAL);

        test.Expect(
            world.TryStartMeleeAttack(
                playerID,
                monsterID,
                1),
            "Miss setup succeeds");

        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COAL),
            coalBefore,
            "Miss grants no rewards");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{9, 1, 0}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(10, 10);

        int coalBefore =
            player->GetInventory().GetItemAmount(ItemType::COAL);

        test.Expect(
            world.TryStartMeleeAttack(
                playerID,
                monsterID,
                1),
            "Zero-damage setup succeeds");

        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COAL),
            coalBefore,
            "Zero-damage attack grants no rewards");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);
        player->GetPosition().SetPosition(10, 10);
        BoostAttackSkill(*player);

        int coalBefore =
            player->GetInventory().GetItemAmount(ItemType::COAL);
        int weightedBefore =
            GetWeightedOreCount(player->GetInventory());

        test.Expect(
            world.TryStartMeleeAttack(
                playerID,
                monsterID,
                1),
            "Lethal attack setup succeeds");

        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COAL),
            coalBefore + 1,
            "Player kill grants guaranteed coal");
        test.ExpectEqual(
            GetWeightedOreCount(player->GetInventory()),
            weightedBefore + 1,
            "Exactly one weighted reward is granted");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COPPER_ORE),
            1,
            "First weighted boundary selects Copper ore");

        const auto &events =
            world.GetEntityDiedEvents();

        test.ExpectEqual(
            static_cast<int>(events.size()),
            1,
            "Death event remains readable after update");

        if (!events.empty())
        {
            test.ExpectEqual(
                events[0].deadEntityID,
                monsterID,
                "Death event records dead monster");
        }
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{61}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            12,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(11, 10);
        BoostAttackSkill(*player);

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::TIN_ORE),
            1,
            "Tin first boundary selects Tin ore");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{100}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            12,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(11, 10);
        BoostAttackSkill(*player);

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::IRON_ORE),
            1,
            "Final weighted boundary selects Iron ore");
    }

    {
        World worldA(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{61}));
        World worldB(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{20, 0, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{61}));

        int playerAID = worldA.CreatePlayer();
        int monsterAID = worldA.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);
        GetPlayer(worldA, playerAID)->GetPosition().SetPosition(10, 10);
        BoostAttackSkill(*GetPlayer(worldA, playerAID));

        int playerBID = worldB.CreatePlayer();
        int monsterBID = worldB.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);
        GetPlayer(worldB, playerBID)->GetPosition().SetPosition(10, 10);
        BoostAttackSkill(*GetPlayer(worldB, playerBID));

        worldA.TryStartMeleeAttack(playerAID, monsterAID, 1);
        worldB.TryStartMeleeAttack(playerBID, monsterBID, 1);

        worldA.Update();
        worldB.Update();

        test.ExpectEqual(
            GetPlayer(worldA, playerAID)
                ->GetInventory()
                .GetItemAmount(ItemType::TIN_ORE),
            1,
            "Reward RNG in world A selects Tin");
        test.ExpectEqual(
            GetPlayer(worldB, playerBID)
                ->GetInventory()
                .GetItemAmount(ItemType::TIN_ORE),
            1,
            "Reward RNG remains independent from combat RNG in world B");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);
        player->GetPosition().SetPosition(10, 10);
        BoostAttackSkill(*player);

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        int coalAfterKill =
            player->GetInventory().GetItemAmount(ItemType::COAL);

        world.Update();
        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COAL),
            coalAfterKill,
            "Configured monster grants rewards exactly once");

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COAL),
            coalAfterKill,
            "Interacting with corpse cannot grant loot again");

        test.Expect(
            world.GetEntityByID(monsterID) != nullptr,
            "Dead monster remains in EntityManager");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1));

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(10, 10);
        BoostAttackSkill(*player);

        int coalBefore =
            player->GetInventory().GetItemAmount(ItemType::COAL);

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COAL),
            coalBefore,
            "Monster without reward table grants nothing");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(80, 80, 1, 30));

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(10, 10);
        player->ApplyDamage(
            player->GetCurrentHealth() - 1);

        world.TryStartMeleeAttack(
            monsterID,
            playerID,
            1);

        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COAL),
            0,
            "Monster killing player grants no monster rewards");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}));

        int killerMonsterID = world.CreateMonster(
            20,
            10,
            MakeRatings(80, 80, 1, 30));
        int deadMonsterID = world.CreateMonster(
            21,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        world.TryStartMeleeAttack(
            killerMonsterID,
            deadMonsterID,
            1);

        world.Update();

        int playerCountWithCoal = 0;

        for (const auto &entity : world.GetEntities())
        {
            Player *player = dynamic_cast<Player *>(entity.get());

            if (player == nullptr)
            {
                continue;
            }

            if (player->GetInventory().GetItemAmount(ItemType::COAL) > 0)
            {
                playerCountWithCoal++;
            }
        }

        test.ExpectEqual(
            playerCountWithCoal,
            0,
            "Monster killing monster grants no player rewards");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}));

        int killerID = world.CreatePlayer();
        int deadID = world.CreatePlayer();

        Player *killer = GetPlayer(world, killerID);
        Player *dead = GetPlayer(world, deadID);

        killer->GetPosition().SetPosition(10, 10);
        dead->GetPosition().SetPosition(11, 10);
        BoostAttackSkill(*killer);
        dead->ApplyDamage(
            dead->GetCurrentHealth() - 1);

        int coalBefore =
            killer->GetInventory().GetItemAmount(ItemType::COAL);

        world.TryStartMeleeAttack(
            killerID,
            deadID,
            1);

        world.Update();

        test.ExpectEqual(
            killer->GetInventory().GetItemAmount(ItemType::COAL),
            coalBefore,
            "Player killing player grants no monster rewards");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(10, 10);
        monster->ApplyDamage(1);

        int coalBefore =
            player->GetInventory().GetItemAmount(ItemType::COAL);

        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COAL),
            coalBefore,
            "Invalid killer death event grants no loot");

        const auto &events =
            world.GetEntityDiedEvents();

        test.ExpectEqual(
            static_cast<int>(events.size()),
            1,
            "Invalid killer death event is still recorded");

        if (!events.empty())
        {
            test.ExpectEqual(
                events[0].killerEntityID,
                EntityDiedEvent::InvalidKillerEntityID,
                "Invalid killer uses the sentinel killer ID");
        }
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(10, 10);
        BoostAttackSkill(*player);

        Inventory &inventory =
            player->GetInventory();

        inventory.AddItem(ItemType::COAL, 1);
        inventory.AddItem(ItemType::LOG, 23);

        int coalBefore =
            inventory.GetItemAmount(ItemType::COAL);

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COAL),
            coalBefore,
            "Atomic failure does not grant Coal when ore cannot fit");
        test.ExpectEqual(
            GetWeightedOreCount(inventory),
            0,
            "Full inventory grants nothing");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2}),
            std::make_unique<InvalidRandomSource>());

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);
        player->GetPosition().SetPosition(10, 10);
        BoostAttackSkill(*player);

        int coalBefore =
            player->GetInventory().GetItemAmount(ItemType::COAL);
        int weightedBefore =
            GetWeightedOreCount(player->GetInventory());

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        test.ExpectEqual(
            static_cast<int>(world.GetEntityDiedEvents().size()),
            1,
            "Failed reward roll still processes the death event once");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COAL),
            coalBefore,
            "Failed reward roll adds no guaranteed item");
        test.ExpectEqual(
            GetWeightedOreCount(player->GetInventory()),
            weightedBefore,
            "Failed reward roll adds no weighted item");
        test.Expect(
            world.GetEntityByID(monsterID) == monster,
            "Dead monster remains in EntityManager after failed reward roll");

        world.Update();
        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COAL),
            coalBefore,
            "Later ticks do not retry a failed reward roll");
        test.ExpectEqual(
            GetWeightedOreCount(player->GetInventory()),
            weightedBefore,
            "Failed reward roll has no later successful grant path");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2, 6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1, 61}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(10, 10);
        BoostAttackSkill(*player);

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        monster->RestoreHealthToFull();

        world.Update();

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COAL),
            2,
            "Revived configured monster can grant rewards again after a new death");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COPPER_ORE),
            1,
            "First death uses first weighted roll");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::TIN_ORE),
            1,
            "Second death uses next weighted roll");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2, 6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1, 61}));

        int firstPlayerID = world.CreatePlayer();
        int secondPlayerID = world.CreatePlayer();

        int firstMonsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);
        int secondMonsterID = world.CreateMonster(
            13,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        Player *firstPlayer = GetPlayer(world, firstPlayerID);
        Player *secondPlayer = GetPlayer(world, secondPlayerID);

        firstPlayer->GetPosition().SetPosition(10, 10);
        secondPlayer->GetPosition().SetPosition(12, 10);
        BoostAttackSkill(*firstPlayer);
        BoostAttackSkill(*secondPlayer);

        world.TryStartMeleeAttack(
            firstPlayerID,
            firstMonsterID,
            1);
        world.TryStartMeleeAttack(
            secondPlayerID,
            secondMonsterID,
            1);

        world.Update();

        const auto &events =
            world.GetEntityDiedEvents();

        test.ExpectEqual(
            static_cast<int>(events.size()),
            2,
            "Two monster deaths in one tick produce two events");

        if (events.size() == 2)
        {
            test.ExpectEqual(
                events[0].deadEntityID,
                firstMonsterID,
                "First death event keeps deterministic order");
            test.ExpectEqual(
                events[1].deadEntityID,
                secondMonsterID,
                "Second death event keeps deterministic order");
        }

        test.ExpectEqual(
            firstPlayer->GetInventory().GetItemAmount(ItemType::COPPER_ORE),
            1,
            "First event consumes first weighted roll");
        test.ExpectEqual(
            secondPlayer->GetInventory().GetItemAmount(ItemType::TIN_ORE),
            1,
            "Second event consumes second weighted roll");
    }

    {
        World world(
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{6, 2, 2}),
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1}));

        int playerID = world.CreatePlayer();

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(5, 1);

        Monster *developmentMonster = nullptr;

        for (const auto &entity : world.GetEntities())
        {
            Monster *candidate =
                dynamic_cast<Monster *>(entity.get());

            if (candidate == nullptr)
            {
                continue;
            }

            if (candidate->GetRewardTableType() ==
                RewardTableType::DEVELOPMENT_MONSTER)
            {
                developmentMonster = candidate;
                break;
            }
        }

        test.Expect(
            developmentMonster != nullptr,
            "Live development monster is assigned development reward table");

        if (developmentMonster != nullptr)
        {
            world.TryStartMeleeAttack(
                playerID,
                developmentMonster->GetID(),
                1);

            developmentMonster->ApplyDamage(
                developmentMonster->GetCurrentHealth() - 1);

            BoostAttackSkill(*player);

            world.Update();

            test.ExpectEqual(
                player->GetInventory().GetItemAmount(ItemType::COAL),
                1,
                "Killing live development monster grants reward table coal");
        }
    }

    return test.Finish();
}
