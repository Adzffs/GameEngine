#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Action/ActionType.h"
#include "../src/Content/ContentValidator.h"
#include "../src/Entity/EntityType.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Player/Player.h"
#include "../src/Skills/SkillType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/Development/DevelopmentWorldContent.h"
#include "../src/World/Distance.h"
#include "../src/World/Event/ActionCancelledEvent.h"
#include "../src/World/World.h"

#include <algorithm>
#include <optional>
#include <vector>

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

    void AdvanceTicks(
        World &world,
        int ticks)
    {
        for (int index = 0; index < ticks; ++index)
        {
            world.Update();
        }
    }

    Monster *FindStarterMonsterByAggression(
        World &world,
        bool aggressive)
    {
        for (const std::unique_ptr<Entity> &entity :
             world.GetEntities())
        {
            Monster *monster =
                dynamic_cast<Monster *>(entity.get());

            if (monster == nullptr)
            {
                continue;
            }

            if (monster->HasAggressionDefinition() == aggressive)
            {
                return monster;
            }
        }

        return nullptr;
    }

    int FindNpcEntityID(World &world)
    {
        for (const std::unique_ptr<Entity> &entity :
             world.GetEntities())
        {
            if (entity->GetType() == EntityType::NPC)
            {
                return entity->GetID();
            }
        }

        return -1;
    }

    void BoostAttack(Player &player)
    {
        player.GetSkills().AddXP(
            SkillType::ATTACK,
            100000);
    }

    bool KillMonsterWithPlayer(
        World &world,
        int playerID,
        int monsterID,
        int maxTicks)
    {
        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        if (player == nullptr ||
            monster == nullptr)
        {
            return false;
        }

        BoostAttack(*player);

        if (monster->GetCurrentHealth() > 1)
        {
            monster->ApplyDamage(
                monster->GetCurrentHealth() - 1);
        }

        for (int tick = 0; tick < maxTicks; ++tick)
        {
            if (!monster->IsAlive())
            {
                return true;
            }

            player->GetPosition().SetPosition(
                monster->GetPosition().GetX() - 1,
                monster->GetPosition().GetY());

            if (world.GetActionForEntity(playerID) == nullptr)
            {
                world.TryStartMeleeAttack(
                    playerID,
                    monsterID,
                    1);
            }

            world.Update();
        }

        return !monster->IsAlive();
    }

    int CountRewardLoot(const Inventory &inventory)
    {
        return inventory.GetItemAmount(ItemType::COAL) +
               inventory.GetItemAmount(ItemType::COPPER_ORE) +
               inventory.GetItemAmount(ItemType::TIN_ORE) +
               inventory.GetItemAmount(ItemType::IRON_ORE);
    }
}

int main()
{
    TestContext test;

    {
        const std::vector<DevelopmentMonsterSpawnDefinition> &starterSpawns =
            DevelopmentWorldContent::GetStarterMonsterSpawns();

        test.ExpectEqual(
            static_cast<int>(starterSpawns.size()),
            2,
            "Starter content defines exactly one passive and one aggressive monster");

        test.Expect(
            !starterSpawns[0].aggressionDefinition.has_value(),
            "Starter passive monster keeps no aggression definition");

        test.Expect(
            starterSpawns[1].aggressionDefinition.has_value(),
            "Starter aggressive monster has aggression definition");

        if (starterSpawns[1].aggressionDefinition.has_value())
        {
            test.ExpectEqual(
                starterSpawns[1].aggressionDefinition->detectionRadius,
                5,
                "Starter aggressive monster uses configured detection radius");
            test.ExpectEqual(
                starterSpawns[1].aggressionDefinition->leashRadius,
                8,
                "Starter aggressive monster uses configured leash radius");
        }

        test.Expect(
            ContentValidator::ValidateAll().IsValid(),
            "Startup validation remains valid with passive and aggressive monsters");
    }

    {
        World world;

        Monster *passiveMonster =
            FindStarterMonsterByAggression(world, false);

        Monster *aggressiveMonster =
            FindStarterMonsterByAggression(world, true);

        test.Expect(
            passiveMonster != nullptr,
            "Passive starter monster exists");
        test.Expect(
            aggressiveMonster != nullptr,
            "Aggressive starter monster exists");

        if (passiveMonster != nullptr &&
            aggressiveMonster != nullptr)
        {
            test.Expect(
                passiveMonster->GetPosition().GetX() != aggressiveMonster->GetPosition().GetX() ||
                    passiveMonster->GetPosition().GetY() != aggressiveMonster->GetPosition().GetY(),
                "Passive and aggressive monsters spawn on distinct tiles");

            test.Expect(
                world.GetMap().IsValidPosition(
                    passiveMonster->GetPosition().GetX(),
                    passiveMonster->GetPosition().GetY()),
                "Passive starter monster spawn tile is walkable");

            test.Expect(
                world.GetMap().IsValidPosition(
                    aggressiveMonster->GetPosition().GetX(),
                    aggressiveMonster->GetPosition().GetY()),
                "Aggressive starter monster spawn tile is walkable");
        }
    }

    {
        World world;

        int playerID = world.CreatePlayer();

        Monster *passiveMonster =
            FindStarterMonsterByAggression(world, false);

        test.Expect(
            passiveMonster != nullptr,
            "Passive monster is available for passive behavior validation");

        if (passiveMonster != nullptr)
        {
            Player *player = GetPlayer(world, playerID);
            player->GetPosition().SetPosition(
                passiveMonster->GetPosition().GetX() - 1,
                passiveMonster->GetPosition().GetY());

            AdvanceTicks(world, 3);

            test.ExpectEqual(
                passiveMonster->GetAggressionTargetEntityID(),
                Monster::InvalidAggressionTargetEntityID,
                "Passive monster never acquires proactive aggression target");

            test.Expect(
                world.GetActionForEntity(passiveMonster->GetID()) == nullptr,
                "Passive monster remains idle when merely approached");
        }
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int aggressiveMonsterID = world.CreateMonster(
            15,
            15,
            CombatRatings{8, 8, 8, 25},
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterRespawnDefinition{8},
            MonsterAggressionDefinition{5, 8});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, aggressiveMonsterID);

        player->GetPosition().SetPosition(30, 30);

        world.Update();

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            Monster::InvalidAggressionTargetEntityID,
            "Aggressive monster ignores players outside detection radius");

        player->GetPosition().SetPosition(18, 15);

        world.Update();

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            playerID,
            "Aggressive monster acquires player inside detection radius");

        player->ApplyDamage(player->GetCurrentHealth());
        world.Update();

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            Monster::InvalidAggressionTargetEntityID,
            "Dead player is not retained as aggression target");
    }

    {
        World world;

        int firstPlayerID = world.CreatePlayer();
        int secondPlayerID = world.CreatePlayer();
        int aggressiveMonsterID = world.CreateMonster(
            10,
            10,
            CombatRatings{8, 8, 8, 25},
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterRespawnDefinition{8},
            MonsterAggressionDefinition{8, 10});

        Player *firstPlayer = GetPlayer(world, firstPlayerID);
        Player *secondPlayer = GetPlayer(world, secondPlayerID);
        Monster *monster = GetMonster(world, aggressiveMonsterID);

        firstPlayer->GetPosition().SetPosition(13, 10);
        secondPlayer->GetPosition().SetPosition(11, 10);

        world.Update();

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            secondPlayerID,
            "Nearest player is selected as deterministic aggression target");

        firstPlayer->GetPosition().SetPosition(8, 10);
        secondPlayer->GetPosition().SetPosition(12, 10);

        world.Update();

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            secondPlayerID,
            "Current valid aggression target is retained and not switched every tick");

        secondPlayer->ApplyDamage(secondPlayer->GetCurrentHealth());
        firstPlayer->GetPosition().SetPosition(11, 10);

        world.Update();

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            firstPlayerID,
            "Aggression reacquires when prior target becomes invalid");
    }

    {
        World world;

        int lowerIDPlayer = world.CreatePlayer();
        int higherIDPlayer = world.CreatePlayer();
        int aggressiveMonsterID = world.CreateMonster(
            40,
            40,
            CombatRatings{8, 8, 8, 25},
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterRespawnDefinition{8},
            MonsterAggressionDefinition{6, 9});

        Player *firstPlayer = GetPlayer(world, lowerIDPlayer);
        Player *secondPlayer = GetPlayer(world, higherIDPlayer);
        Monster *monster = GetMonster(world, aggressiveMonsterID);

        firstPlayer->GetPosition().SetPosition(42, 40);
        secondPlayer->GetPosition().SetPosition(40, 42);

        world.Update();

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            lowerIDPlayer,
            "Equal-distance tie breaks toward lower entity ID");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int aggressiveMonsterID = world.CreateMonster(
            20,
            20,
            CombatRatings{8, 8, 8, 25},
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterRespawnDefinition{8},
            MonsterAggressionDefinition{8, 12});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, aggressiveMonsterID);

        player->GetPosition().SetPosition(24, 20);

        const int startX = monster->GetPosition().GetX();
        const int startY = monster->GetPosition().GetY();

        world.Update();
        world.Update();

        const int movedX = monster->GetPosition().GetX();
        const int movedY = monster->GetPosition().GetY();

        test.Expect(
            world.GetActionForEntity(aggressiveMonsterID) == nullptr,
            "Aggressive monster does not start melee action while out of range");

        test.Expect(
            movedX != startX || movedY != startY,
            "Aggressive monster starts chasing target via movement system");

        test.Expect(
            std::abs(movedX - startX) <= 1 &&
                std::abs(movedY - startY) <= 1,
            "Aggressive chase moves no more than one tile step per tick");

        test.Expect(
            world.GetMap().IsValidPosition(movedX, movedY),
            "Aggressive chase never moves monster onto blocked map tiles");

        for (int step = 0; step < 20; ++step)
        {
            world.Update();

            if (world.GetActionForEntity(aggressiveMonsterID) != nullptr)
            {
                break;
            }
        }

        const Action *monsterAction =
            world.GetActionForEntity(aggressiveMonsterID);

        test.Expect(
            monsterAction != nullptr,
            "Aggressive monster starts melee engagement when adjacent");

        if (monsterAction != nullptr)
        {
            test.ExpectEqual(
                static_cast<int>(monsterAction->GetType()),
                static_cast<int>(ActionType::MELEE_ATTACK),
                "Aggressive melee uses existing timed melee action type");
            test.Expect(
                monsterAction->IsRepeating(),
                "Aggressive melee uses repeating engagement action");
            test.ExpectEqual(
                monsterAction->GetDuration(),
                5,
                "Aggressive melee keeps existing monster attack cadence");
        }

        test.Expect(
            monster->GetPosition().GetX() != player->GetPosition().GetX() ||
                monster->GetPosition().GetY() != player->GetPosition().GetY(),
            "Chasing stops on an adjacent tile instead of entering the target tile");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int aggressiveMonsterID = world.CreateMonster(
            70,
            70,
            CombatRatings{8, 8, 8, 25},
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterRespawnDefinition{8},
            MonsterAggressionDefinition{5, 20});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, aggressiveMonsterID);

        player->GetPosition().SetPosition(0, 0);
        monster->GetPosition().SetPosition(80, 70);

        world.Update();

        const int xAfterReturnStep =
            monster->GetPosition().GetX();

        player->GetPosition().SetPosition(82, 70);

        world.Update();

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            playerID,
            "Aggressive monster can acquire while returning home");

        world.Update();

        test.Expect(
            monster->GetPosition().GetX() >=
                xAfterReturnStep,
            "Acquiring a player interrupts return-home path instead of continuing old return direction");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int aggressiveMonsterID = world.CreateMonster(
            60,
            60,
            CombatRatings{8, 8, 8, 25},
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterRespawnDefinition{8},
            MonsterAggressionDefinition{8, 12});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, aggressiveMonsterID);

        player->GetPosition().SetPosition(61, 60);

        const int healthBefore = player->GetCurrentHealth();

        world.Update();

        test.Expect(
            world.GetActionForEntity(aggressiveMonsterID) != nullptr,
            "Aggressive monster starts engagement in range");

        test.ExpectEqual(
            player->GetCurrentHealth(),
            healthBefore,
            "Aggression acquisition does not apply immediate direct damage");

        const int targetBefore =
            monster->GetAggressionTargetEntityID();

        world.Update();

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            targetBefore,
            "Aggressive retaliation path does not create duplicate target ownership");

        AdvanceTicks(world, 20);

        test.Expect(
            player->GetCurrentHealth() < healthBefore,
            "Existing melee action resolver eventually applies damage to the player");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int aggressiveMonsterID = world.CreateMonster(
            70,
            70,
            CombatRatings{8, 8, 8, 25},
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterRespawnDefinition{8},
            MonsterAggressionDefinition{5, 6});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, aggressiveMonsterID);

        player->GetPosition().SetPosition(71, 70);

        world.Update();

        test.Expect(
            world.GetActionForEntity(aggressiveMonsterID) != nullptr,
            "Aggressive monster starts an in-range melee action before leash-break cleanup test");

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            playerID,
            "Aggressive monster initially acquires a valid target");

        monster->GetPosition().SetPosition(80, 70);
        world.Update();

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            Monster::InvalidAggressionTargetEntityID,
            "Leash violation clears aggression target");

        test.Expect(
            world.GetActionForEntity(aggressiveMonsterID) == nullptr,
            "Leash violation cancels stale monster melee action through existing cancellation path");

        player->GetPosition().SetPosition(0, 0);

        const int healthBeforeReturn =
            monster->GetCurrentHealth();

        for (int step = 0; step < 20; ++step)
        {
            world.Update();

            if (monster->GetPosition().GetX() == monster->GetOriginalSpawnX() &&
                monster->GetPosition().GetY() == monster->GetOriginalSpawnY())
            {
                break;
            }
        }

        test.ExpectEqual(
            monster->GetPosition().GetX(),
            monster->GetOriginalSpawnX(),
            "Leashed aggressive monster returns toward original spawn X");
        test.ExpectEqual(
            monster->GetPosition().GetY(),
            monster->GetOriginalSpawnY(),
            "Leashed aggressive monster returns toward original spawn Y");

        test.ExpectEqual(
            monster->GetCurrentHealth(),
            healthBeforeReturn,
            "Returning home does not heal aggressive monster health");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int aggressiveMonsterID = world.CreateMonster(
            25,
            25,
            CombatRatings{8, 8, 8, 12},
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterRespawnDefinition{2},
            MonsterAggressionDefinition{8, 12});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, aggressiveMonsterID);

        player->GetPosition().SetPosition(24, 25);

        world.Update();

        const int lootBefore =
            CountRewardLoot(player->GetInventory());

        test.Expect(
            KillMonsterWithPlayer(
                world,
                playerID,
                aggressiveMonsterID,
                40),
            "Aggressive monster can be killed through existing player melee flow");

        test.Expect(
            !world.GetEntityDiedEvents().empty(),
            "Aggressive monster death emits existing death event");

        test.Expect(
            world.HasScheduledMonsterRespawn(aggressiveMonsterID),
            "Aggressive monster death schedules deterministic respawn");

        const int lootAfterKill =
            CountRewardLoot(player->GetInventory());

        test.Expect(
            lootAfterKill > lootBefore,
            "Aggressive monster death grants loot once through reward table");

        AdvanceTicks(world, 2);

        test.Expect(
            monster->IsAlive(),
            "Aggressive monster respawns after configured delay");
        test.ExpectEqual(
            monster->GetID(),
            aggressiveMonsterID,
            "Aggressive monster respawns using same entity ID");
        test.ExpectEqual(
            monster->GetPosition().GetX(),
            monster->GetOriginalSpawnX(),
            "Aggressive monster respawns at original X position");
        test.ExpectEqual(
            monster->GetPosition().GetY(),
            monster->GetOriginalSpawnY(),
            "Aggressive monster respawns at original Y position");
        test.ExpectEqual(
            monster->GetCurrentHealth(),
            monster->GetMaximumHealth(),
            "Aggressive monster respawn restores full health");

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            Monster::InvalidAggressionTargetEntityID,
            "Aggressive monster respawns with cleared aggression target");

        test.Expect(
            world.GetActionForEntity(aggressiveMonsterID) == nullptr,
            "Aggressive monster respawn has no stale melee action");

        player->GetPosition().SetPosition(
            monster->GetPosition().GetX() + 1,
            monster->GetPosition().GetY());

        world.Update();

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            playerID,
            "Aggressive monster can reacquire a target after respawn");
    }

    {
        World world;

        int npcID = FindNpcEntityID(world);
        int playerID = world.CreatePlayer();
        int aggressiveMonsterID = world.CreateMonster(
            45,
            45,
            CombatRatings{8, 8, 8, 20},
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterRespawnDefinition{8},
            MonsterAggressionDefinition{6, 10});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, aggressiveMonsterID);

        test.Expect(
            npcID > 0,
            "Starter NPC exists for non-player aggression filter validation");

        if (npcID > 0)
        {
            Entity *npc = world.GetEntityByID(npcID);
            npc->GetPosition().SetPosition(46, 45);
        }

        player->GetPosition().SetPosition(47, 45);

        world.Update();

        test.ExpectEqual(
            monster->GetAggressionTargetEntityID(),
            playerID,
            "Aggressive scan ignores non-player entities");
    }

    return test.Finish();
}
