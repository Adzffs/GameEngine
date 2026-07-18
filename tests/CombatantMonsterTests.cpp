#include "TestSupport.h"

#include "../src/Action/ActionType.h"
#include "../src/Combat/Combatant.h"
#include "../src/Entity/Entity.h"
#include "../src/Entity/EntityType.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Movement/MovementRequest.h"
#include "../src/Player/Player.h"
#include "../src/Recipe/RecipeType.h"
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

    Combatant *GetCombatant(
        World &world,
        int entityID)
    {
        Entity *entity =
            world.GetEntityByID(entityID);

        if (entity == nullptr)
        {
            return nullptr;
        }

        return dynamic_cast<Combatant *>(entity);
    }

    int FindInventorySlot(
        const Inventory &inventory,
        ItemType itemType)
    {
        const auto &slots = inventory.GetSlots();

        for (int index = 0; index < static_cast<int>(slots.size()); ++index)
        {
            if (!slots[index].IsEmpty() &&
                slots[index].GetItemType() == itemType)
            {
                return index;
            }
        }

        return -1;
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

        player->GetInventory().AddItem(itemType, 1);

        int slotIndex = FindInventorySlot(
            player->GetInventory(),
            itemType);

        if (slotIndex < 0)
        {
            return false;
        }

        return world.TryEquipInventoryItem(
            playerID,
            slotIndex);
    }

    int FindNpcEntityID(
        World &world)
    {
        for (const auto &entity : world.GetEntities())
        {
            if (entity->GetType() == EntityType::NPC)
            {
                return entity->GetID();
            }
        }

        return -1;
    }

    bool AreEqual(
        const CombatRatings &left,
        const CombatRatings &right)
    {
        return left.attackAccuracy == right.attackAccuracy &&
               left.meleeStrength == right.meleeStrength &&
               left.defence == right.defence &&
               left.maximumHealth == right.maximumHealth;
    }

    bool AreEqual(
        const MeleeAttackResult &left,
        const MeleeAttackResult &right)
    {
        return left.attackerMaximumRoll == right.attackerMaximumRoll &&
               left.defenderMaximumRoll == right.defenderMaximumRoll &&
               left.attackerRolledResult == right.attackerRolledResult &&
               left.defenderRolledResult == right.defenderRolledResult &&
               left.maximumHit == right.maximumHit &&
               left.didHit == right.didHit &&
               left.rolledDamage == right.rolledDamage &&
               left.actualDamageApplied == right.actualDamageApplied;
    }

    void OpenFurnace(
        TestContext &test,
        World &world,
        int playerID)
    {
        CraftingStation *station =
            world.GetStationAt(14, 9);

        test.Expect(
            station != nullptr,
            "Furnace exists for unrelated-action combat test");

        if (station == nullptr)
        {
            return;
        }

        world.QueueStationInteraction(
            playerID,
            station->GetID());

        world.Update();
    }
}

int main()
{
    TestContext test;

    {
        Player player(5001);
        Combatant *combatant =
            dynamic_cast<Combatant *>(&player);

        test.Expect(
            combatant != nullptr,
            "Player is usable through Combatant interface");
        test.Expect(
            combatant->IsAlive(),
            "Player combatant starts alive");
        test.ExpectEqual(
            combatant->GetCurrentHealth(),
            combatant->GetMaximumHealth(),
            "Player combatant starts at full health");
    }

    {
        CombatRatings requestedRatings{9, 11, 7, 0};
        Monster monster(5002, 4, 6, requestedRatings);
        Combatant *combatant =
            dynamic_cast<Combatant *>(&monster);

        test.Expect(
            combatant != nullptr,
            "Monster is usable through Combatant interface");
        test.ExpectEqual(
            combatant->GetMaximumHealth(),
            1,
            "Monster maximum health is safely clamped through HealthPool");
        test.ExpectEqual(
            combatant->GetCurrentHealth(),
            combatant->GetMaximumHealth(),
            "Monster starts at full health");
        test.ExpectEqual(
            monster.GetCombatRatings().attackAccuracy,
            requestedRatings.attackAccuracy,
            "Monster preserves configured attack accuracy");
        test.ExpectEqual(
            monster.GetCombatRatings().meleeStrength,
            requestedRatings.meleeStrength,
            "Monster preserves configured melee strength");
        test.ExpectEqual(
            monster.GetCombatRatings().defence,
            requestedRatings.defence,
            "Monster preserves configured defence");
        test.ExpectEqual(
            monster.GetCombatRatings().maximumHealth,
            1,
            "Monster ratings maximum health is normalized to clamped health cap");

        test.ExpectEqual(
            combatant->ApplyDamage(1),
            1,
            "Monster combatant supports damage application");
        test.Expect(
            !combatant->IsAlive(),
            "Monster combatant transitions to dead after lethal damage");
    }

    {
        World world;

        CombatRatings firstRatings{5, 6, 4, 12};
        CombatRatings secondRatings{7, 8, 3, 22};

        int firstMonsterID =
            world.CreateMonster(20, 21, firstRatings);
        int secondMonsterID =
            world.CreateMonster(22, 23, secondRatings);

        test.Expect(
            firstMonsterID != secondMonsterID,
            "World assigns unique entity IDs to monsters");

        Monster *first =
            GetMonster(world, firstMonsterID);

        Monster *second =
            GetMonster(world, secondMonsterID);

        test.Expect(
            first != nullptr,
            "Created monster is retrievable by world entity lookup");
        test.Expect(
            second != nullptr,
            "Multiple created monsters coexist in world entity ownership");

        test.ExpectEqual(
            first->GetPosition().GetX(),
            20,
            "Created monster stores spawn X position");
        test.ExpectEqual(
            first->GetPosition().GetY(),
            21,
            "Created monster stores spawn Y position");
    }

    {
        World world(701U);

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            CombatRatings{8, 8, 8, 40});

        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(10, 10);

        test.Expect(
            world.TryStartMeleeAttack(
                playerID,
                monsterID,
                1),
            "Player can attack adjacent monster");

        world.Update();

        test.Expect(
            world.GetLastMeleeAttackResult().has_value(),
            "Player attacking monster resolves through world melee flow");
    }

    {
        World world(702U);

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            10,
            10,
            CombatRatings{8, 8, 8, 40});

        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(11, 10);

        test.Expect(
            world.TryStartMeleeAttack(
                monsterID,
                playerID,
                1),
            "Monster can attack adjacent player");

        world.Update();

        test.Expect(
            world.GetLastMeleeAttackResult().has_value(),
            "Monster attacking player resolves through world melee flow");
    }

    {
        World world(703U);

        int monsterAID = world.CreateMonster(
            10,
            10,
            CombatRatings{10, 10, 10, 45});
        int monsterBID = world.CreateMonster(
            11,
            10,
            CombatRatings{7, 7, 7, 35});

        test.Expect(
            world.TryStartMeleeAttack(
                monsterAID,
                monsterBID,
                1),
            "Monster can attack adjacent monster");

        world.Update();

        test.Expect(
            world.GetLastMeleeAttackResult().has_value(),
            "Monster attacking monster resolves through world melee flow");
    }

    {
        World world(704U);

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                1),
            "Player attacking player remains supported");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int farMonsterID = world.CreateMonster(
            40,
            40,
            CombatRatings{8, 8, 8, 30});

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(10, 10);

        test.Expect(
            !world.TryStartMeleeAttack(
                playerID,
                farMonsterID,
                1),
            "Out-of-range combatant combinations are rejected");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            CombatRatings{9, 9, 9, 20});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(10, 10);
        monster->ApplyDamage(10000);

        test.Expect(
            !world.TryStartMeleeAttack(
                monsterID,
                playerID,
                1),
            "Dead monster cannot attack");
        test.Expect(
            !world.TryStartMeleeAttack(
                playerID,
                monsterID,
                1),
            "Dead monster cannot be targeted");
    }

    {
        World world(710U);

        int monsterID = world.CreateMonster(
            10,
            10,
            CombatRatings{12, 12, 7, 40});
        int playerID = world.CreatePlayer();

        Monster *monster = GetMonster(world, monsterID);
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(11, 10);

        int playerHealthBefore =
            player->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeAttack(
                monsterID,
                playerID,
                2),
            "Monster melee starts for completion timing validation");

        test.ExpectEqual(
            player->GetCurrentHealth(),
            playerHealthBefore,
            "No damage is applied immediately on melee start");

        world.Update();

        test.ExpectEqual(
            player->GetCurrentHealth(),
            playerHealthBefore,
            "No damage is applied before completion tick");

        world.Update();

        test.Expect(
            world.GetLastMeleeAttackResult().has_value(),
            "Exactly one melee resolution occurs on completion");

        MeleeAttackResult firstResult =
            world.GetLastMeleeAttackResult().value();

        test.Expect(
            world.GetActionForEntity(monsterID) == nullptr,
            "Melee action does not repeat automatically after completion");

        world.Update();

        test.Expect(
            world.GetLastMeleeAttackResult().has_value(),
            "Last melee result remains observable after completion");
        test.Expect(
            AreEqual(
                world.GetLastMeleeAttackResult().value(),
                firstResult),
            "No additional resolution occurs after non-repeating completion");
    }

    {
        World world(711U);

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            CombatRatings{10, 10, 10, 40});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        player->GetPosition().SetPosition(10, 10);

        CombatRatings monsterRatingsBefore =
            monster->GetCombatRatings();
        CombatRatings playerRatingsBefore =
            player->GetCombatRatings();

        test.Expect(
            EquipItem(
                world,
                playerID,
                ItemType::BRONZE_SWORD),
            "Player can equip bronze sword for combat rating coverage");

        CombatRatings playerRatingsAfterEquip =
            player->GetCombatRatings();

        test.Expect(
            playerRatingsAfterEquip.attackAccuracy >= playerRatingsBefore.attackAccuracy,
            "Player equipment affects combat ratings");
        test.Expect(
            playerRatingsAfterEquip.meleeStrength >= playerRatingsBefore.meleeStrength,
            "Player equipment can affect melee strength");
        test.Expect(
            AreEqual(
                monster->GetCombatRatings(),
                monsterRatingsBefore),
            "Monster fixed ratings are unchanged by player equipment");

        test.Expect(
            world.TryStartMeleeAttack(
                playerID,
                monsterID,
                1),
            "Equipped player can still attack monster");

        world.Update();

        test.Expect(
            AreEqual(
                monster->GetCombatRatings(),
                monsterRatingsBefore),
            "Monster fixed ratings remain unchanged after combat resolution");
    }

    {
        World world(712U);

        int defenderID = world.CreatePlayer();
        int attackerMonsterID = world.CreateMonster(
            12,
            9,
            CombatRatings{9, 9, 9, 40});

        Player *defender = GetPlayer(world, defenderID);

        defender->GetPosition().SetPosition(13, 9);

        OpenFurnace(test, world, defenderID);

        defender->GetInventory().AddItem(ItemType::COPPER_ORE, 2);
        defender->GetInventory().AddItem(ItemType::TIN_ORE, 2);

        test.Expect(
            world.TryStartRecipeAction(
                defenderID,
                RecipeType::BRONZE_BAR),
            "Defender starts unrelated repeating smithing action");

        test.Expect(
            world.TryStartMeleeAttack(
                attackerMonsterID,
                defenderID,
                1),
            "Monster starts melee against defender with unrelated action");

        world.Update();

        test.Expect(
            world.GetActionForEntity(defenderID) != nullptr,
            "Defender unrelated action remains active when targeted by melee");
    }

    {
        World world(713U);

        int monsterID = world.CreateMonster(
            10,
            10,
            CombatRatings{10, 10, 10, 40});
        int playerID = world.CreatePlayer();

        Monster *monster = GetMonster(world, monsterID);
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(11, 10);

        int playerHealthBefore =
            player->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeAttack(
                monsterID,
                playerID,
                2),
            "Melee starts for defender-death completion revalidation");

        player->ApplyDamage(10000);

        AdvanceWorldTicks(world, 2);

        test.Expect(
            !world.GetLastMeleeAttackResult().has_value(),
            "No resolution occurs when defender dies before completion");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            0,
            "Dead defender remains at zero health");
        test.ExpectEqual(
            playerHealthBefore,
            player->GetMaximumHealth(),
            "Defender started alive before pre-completion death");

        monster->ApplyDamage(0);
    }

    {
        World world(714U);

        int monsterID = world.CreateMonster(
            10,
            10,
            CombatRatings{10, 10, 10, 40});
        int playerID = world.CreatePlayer();

        Monster *monster = GetMonster(world, monsterID);
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(11, 10);

        int playerHealthBefore =
            player->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeAttack(
                monsterID,
                playerID,
                2),
            "Melee starts for attacker-death completion revalidation");

        monster->ApplyDamage(10000);

        AdvanceWorldTicks(world, 2);

        test.Expect(
            !world.GetLastMeleeAttackResult().has_value(),
            "No resolution occurs when attacker dies before completion");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            playerHealthBefore,
            "Dead attacker cannot apply damage on completion");
    }

    {
        World world(715U);

        int monsterID = world.CreateMonster(
            10,
            10,
            CombatRatings{10, 10, 10, 40});
        int playerID = world.CreatePlayer();

        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(11, 10);

        int playerHealthBefore =
            player->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeAttack(
                monsterID,
                playerID,
                2),
            "Melee starts for out-of-range completion revalidation");

        player->GetPosition().SetPosition(50, 50);

        AdvanceWorldTicks(world, 2);

        test.Expect(
            !world.GetLastMeleeAttackResult().has_value(),
            "No resolution occurs if movable player target leaves range before completion");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            playerHealthBefore,
            "Out-of-range completion revalidation prevents damage");
    }

    {
        World world;

        int npcID = FindNpcEntityID(world);
        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            CombatRatings{8, 8, 8, 30});

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(10, 10);

        test.Expect(
            npcID >= 0,
            "World exposes at least one non-combat NPC entity for safe rejection tests");

        test.Expect(
            !world.TryStartMeleeAttack(
                npcID,
                playerID,
                1),
            "Non-combat attacker entity is rejected safely");

        test.Expect(
            !world.TryStartMeleeAttack(
                playerID,
                npcID,
                1),
            "Non-combat defender entity is rejected safely");

        test.Expect(
            !world.TryStartMeleeAttack(
                playerID,
                999999,
                1),
            "Missing target entity is rejected safely");

        test.Expect(
            !world.TryStartMeleeAttack(
                999998,
                monsterID,
                1),
            "Missing attacker entity is rejected safely");
    }

    {
        World world(716U);

        int attackerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            CombatRatings{8, 8, 8, 40});

        Player *attacker = GetPlayer(world, attackerID);
        Monster *monster = GetMonster(world, monsterID);

        attacker->GetPosition().SetPosition(10, 10);

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                monsterID,
                3),
            "Melee starts for movement-request cancellation semantics");

        MovementRequest movement(
            attackerID,
            1,
            0);

        world.QueueMovementRequest(movement);

        AdvanceWorldTicks(world, 3);

        test.Expect(
            !world.GetLastMeleeAttackResult().has_value(),
            "Movement request cancels attacker action and prevents melee resolution");
        test.Expect(
            world.GetActionForEntity(attackerID) == nullptr,
            "Owner has no active melee action after movement-request cancellation");

        monster->ApplyDamage(0);
    }

    return test.Finish();
}
