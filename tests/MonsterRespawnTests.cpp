#include "TestSupport.h"

#include "../src/Core/RandomSource.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Player/Player.h"
#include "../src/Reward/RewardTableType.h"
#include "../src/Skills/SkillType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/World.h"

#include <limits>
#include <memory>
#include <utility>
#include <vector>

struct WorldTestAccess
{
    static bool ScheduleMonsterRespawn(
        World &world,
        int entityID,
        int delayTicks)
    {
        int respawnTick = 0;

        if (!world.TryCalculateRespawnTick(
                delayTicks,
                respawnTick))
        {
            return false;
        }

        return world.ScheduleMonsterRespawn(
            entityID,
            respawnTick);
    }
};

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

            int value = values[nextIndex++];

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

    void AdvanceTicks(
        World &world,
        int count)
    {
        for (int index = 0; index < count; ++index)
        {
            world.Update();
        }
    }

    void BoostAttack(
        Player &player)
    {
        player.GetSkills().AddXP(
            SkillType::ATTACK,
            100000);
    }

    int GetLootTotal(
        const Inventory &inventory)
    {
        return inventory.GetItemAmount(ItemType::COAL) +
               inventory.GetItemAmount(ItemType::COPPER_ORE) +
               inventory.GetItemAmount(ItemType::TIN_ORE) +
               inventory.GetItemAmount(ItemType::IRON_ORE);
    }

    bool KillMonsterWithPlayer(
        World &world,
        int playerID,
        int monsterID)
    {
        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        if (player == nullptr ||
            monster == nullptr)
        {
            return false;
        }

        BoostAttack(*player);

        player->GetPosition().SetPosition(
            monster->GetPosition().GetX() - 1,
            monster->GetPosition().GetY());

        if (!world.TryStartMeleeAttack(
                playerID,
                monsterID,
                1))
        {
            return false;
        }

        world.Update();
        return !monster->IsAlive();
    }
}

int main()
{
    TestContext test;

    {
        World world;

        int monsterID = world.CreateMonster(
            20,
            20,
            MakeRatings(1, 1, 1, 1));

        Monster *monster = GetMonster(world, monsterID);
        monster->ApplyDamage(monster->GetCurrentHealth());

        world.Update();

        test.ExpectEqual(
            world.GetScheduledMonsterRespawnCount(),
            0,
            "Generic monster defaults to no automatic respawn");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            22,
            10,
            MakeRatings(1, 1, 1, 2),
            RewardTableType::NONE,
            MonsterRespawnDefinition{3});

        Monster *monster = GetMonster(world, monsterID);

        monster->ApplyDamage(monster->GetCurrentHealth());
        world.Update();

        test.Expect(
            world.HasScheduledMonsterRespawn(monsterID),
            "Configured monster schedules exactly one respawn after death");

        int scheduledTick =
            world.GetScheduledMonsterRespawnTick(monsterID).value_or(-1);

        test.ExpectEqual(
            scheduledTick,
            world.GetCurrentTick() + 3,
            "Respawn due tick is derived from current tick plus delay");

        AdvanceTicks(world, 1);

        test.Expect(
            !monster->IsAlive(),
            "Monster remains dead before due tick");

        test.Expect(
            world.GetEntityByID(monsterID) != nullptr,
            "Dead monster remains in EntityManager while dead");

        test.Expect(
            !world.QueueMeleeEngagementRequest(
                playerID,
                monsterID,
                1),
            "Corpse cannot start a new melee interaction");

        test.ExpectEqual(
            world.GetScheduledMonsterRespawnCount(),
            1,
            "Repeated dead ticks do not create duplicate schedules");

        AdvanceTicks(world, 1);

        test.Expect(
            !monster->IsAlive(),
            "Monster does not respawn one tick early");

        world.Update();

        test.Expect(
            monster->IsAlive(),
            "Monster respawns exactly on configured tick");
        test.ExpectEqual(
            monster->GetID(),
            monsterID,
            "Respawn uses the same entity ID");
        test.ExpectEqual(
            monster->GetCurrentHealth(),
            monster->GetMaximumHealth(),
            "Respawn restores health to maximum");
        test.ExpectEqual(
            monster->GetPosition().GetX(),
            monster->GetOriginalSpawnX(),
            "Respawn returns monster to original X");
        test.ExpectEqual(
            monster->GetPosition().GetY(),
            monster->GetOriginalSpawnY(),
            "Respawn returns monster to original Y");
        test.Expect(
            world.GetEntityDiedEvents().empty(),
            "Respawning does not generate a death event");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
                        std::vector<int>{6, 2, 2}),
                    std::make_unique<SequenceRandomSource>(
                        std::vector<int>{1, 1, 1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            24,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterRespawnDefinition{2});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        const int originalSpawnX =
            monster->GetOriginalSpawnX();
        const int originalSpawnY =
            monster->GetOriginalSpawnY();

        monster->GetPosition().SetPosition(25, 10);

        int lootBefore =
            GetLootTotal(player->GetInventory());

        test.Expect(
            KillMonsterWithPlayer(
                world,
                playerID,
                monsterID),
            "Setup kill succeeds for loot validation");

        int lootAfterDeath =
            GetLootTotal(player->GetInventory());

        test.ExpectEqual(
            lootAfterDeath,
            lootBefore + 2,
            "Death grants exactly one loot roll");

        world.Update();
        world.Update();

        test.ExpectEqual(
            GetLootTotal(player->GetInventory()),
            lootAfterDeath,
            "Respawn does not grant extra loot");

        test.ExpectEqual(
            monster->GetPosition().GetX(),
            originalSpawnX,
            "Moving monster before death does not change spawn X");
        test.ExpectEqual(
            monster->GetPosition().GetY(),
            originalSpawnY,
            "Moving monster before death does not change spawn Y");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
                        std::vector<int>{6, 2, 2}),
                    std::make_unique<SequenceRandomSource>(
                        std::vector<int>{1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            26,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterRespawnDefinition{1});

        Player *player = GetPlayer(world, playerID);

        test.Expect(
            KillMonsterWithPlayer(
                world,
                playerID,
                monsterID),
            "Setup kill succeeds for stale state checks");

        test.Expect(
            world.GetMeleeCombatFeedbacks().count(monsterID) == 1,
            "Combat feedback exists for dead monster before respawn");

        world.Update();

        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Stale actions targeting monster are absent after respawn");
        test.Expect(
            world.GetMeleeCombatFeedbacks().count(monsterID) == 0,
            "Combat feedback involving monster is cleared on respawn");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            40,
            40,
            MakeRatings(1, 1, 1, 2),
            RewardTableType::NONE,
            MonsterRespawnDefinition{2});

        test.Expect(
            world.QueueMeleeEngagementRequest(
                playerID,
                monsterID,
                1),
            "Setup creates pending melee interaction");

        Monster *monster = GetMonster(world, monsterID);
        monster->ApplyDamage(monster->GetCurrentHealth());

        world.Update();

        test.Expect(
            !world.HasPendingMeleeEngagement(playerID),
            "Pending melee interactions involving monster are cleared on death/respawn lifecycle");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
                        std::vector<int>{6, 2, 2, 6, 2, 2}),
                    std::make_unique<SequenceRandomSource>(
                        std::vector<int>{1, 1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            28,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterRespawnDefinition{2});

        Player *player = GetPlayer(world, playerID);

        int initialLoot =
            GetLootTotal(player->GetInventory());

        test.Expect(
            KillMonsterWithPlayer(
                world,
                playerID,
                monsterID),
            "First kill succeeds");

        const int afterFirstKillLoot =
            GetLootTotal(player->GetInventory());

        AdvanceTicks(world, 2);

        test.Expect(
            KillMonsterWithPlayer(
                world,
                playerID,
                monsterID),
            "Second kill after respawn succeeds");

        const auto &events =
            world.GetEntityDiedEvents();

        test.ExpectEqual(
            static_cast<int>(events.size()),
            1,
            "Next death after respawn produces one death event");

        test.ExpectEqual(
            GetLootTotal(player->GetInventory()),
            afterFirstKillLoot + 2,
            "Next death grants one new loot roll");

        test.ExpectEqual(
            world.GetScheduledMonsterRespawnCount(),
            1,
            "Next death schedules exactly one new respawn");

        test.ExpectEqual(
            afterFirstKillLoot,
            initialLoot + 2,
            "First death grants one loot roll");
    }

    {
        World world;

        int firstMonsterID = world.CreateMonster(
            30,
            30,
            MakeRatings(1, 1, 1, 2),
            RewardTableType::NONE,
            MonsterRespawnDefinition{2});
        int secondMonsterID = world.CreateMonster(
            31,
            30,
            MakeRatings(1, 1, 1, 2),
            RewardTableType::NONE,
            MonsterRespawnDefinition{2});

        Monster *firstMonster =
            GetMonster(world, firstMonsterID);
        Monster *secondMonster =
            GetMonster(world, secondMonsterID);

        firstMonster->ApplyDamage(firstMonster->GetCurrentHealth());
        secondMonster->ApplyDamage(secondMonster->GetCurrentHealth());

        world.Update();

        test.ExpectEqual(
            world.GetScheduledMonsterRespawnCount(),
            2,
            "Two dead monsters due on same tick are both scheduled");

        world.Update();

        test.Expect(
            !firstMonster->IsAlive() &&
                !secondMonster->IsAlive(),
            "Neither monster respawns one tick early on shared due tick");

        world.Update();

        test.Expect(
            firstMonster->IsAlive() &&
                secondMonster->IsAlive(),
            "Two monsters due on same tick both respawn deterministically");
    }

    {
        World world;

        int monsterID = world.CreateMonster(
            32,
            32,
            MakeRatings(1, 1, 1, 10),
            RewardTableType::NONE,
            MonsterRespawnDefinition{2});

        Monster *monster = GetMonster(world, monsterID);
        monster->GetPosition().SetPosition(35, 35);

        test.Expect(
            WorldTestAccess::ScheduleMonsterRespawn(
                world,
                monsterID,
                1),
            "Setup stale schedule on alive monster succeeds");

        world.Update();

        test.Expect(
            monster->IsAlive(),
            "Already-alive monster stale respawn is discarded safely");
        test.ExpectEqual(
            monster->GetPosition().GetX(),
            35,
            "Already-alive stale schedule does not teleport monster X");
        test.ExpectEqual(
            monster->GetPosition().GetY(),
            35,
            "Already-alive stale schedule does not teleport monster Y");
        test.ExpectEqual(
            world.GetScheduledMonsterRespawnCount(),
            0,
            "Stale already-alive schedule is removed");
    }

    {
        World world;

        int playerID = world.CreatePlayer();

        test.Expect(
            WorldTestAccess::ScheduleMonsterRespawn(
                world,
                999999,
                1),
            "Missing entity schedule can be queued for stale handling test");
        test.Expect(
            WorldTestAccess::ScheduleMonsterRespawn(
                world,
                playerID,
                1),
            "Non-monster entity schedule can be queued for stale handling test");

        world.Update();

        test.ExpectEqual(
            world.GetScheduledMonsterRespawnCount(),
            0,
            "Missing or invalid scheduled entities are discarded safely");
        test.Expect(
            world.GetEntityByID(playerID) != nullptr,
            "Invalid scheduled entity handling does not break world update");
    }

    {
        World world;

        int monsterID = world.CreateMonster(
            34,
            34,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::NONE,
            MonsterRespawnDefinition{-4});

        Monster *monster = GetMonster(world, monsterID);
        monster->ApplyDamage(monster->GetCurrentHealth());

        world.Update();

        test.ExpectEqual(
            world.GetScheduledMonsterRespawnCount(),
            0,
            "Invalid negative respawn delay is rejected safely");

        test.Expect(
            !WorldTestAccess::ScheduleMonsterRespawn(
                world,
                monsterID,
                -1),
            "Explicit negative scheduling request is rejected safely");
    }

    {
        World world;

        int monsterID = world.CreateMonster(
            36,
            36,
            MakeRatings(1, 1, 1, 10),
            RewardTableType::NONE,
            MonsterRespawnDefinition{2});

        world.Update();

        test.Expect(
            !WorldTestAccess::ScheduleMonsterRespawn(
                world,
                monsterID,
                std::numeric_limits<int>::max()),
            "Tick-overflow scheduling is rejected safely");
    }

    return test.Finish();
}
