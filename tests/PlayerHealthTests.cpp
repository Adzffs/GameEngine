#include "TestSupport.h"

#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Player/Player.h"
#include "../src/Skills/SkillType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/Stats/HealthPool.h"
#include "../src/Stats/StatType.h"
#include "../src/World/World.h"

namespace
{
    Player *GetPlayer(
        World &world,
        int playerID)
    {
        return dynamic_cast<Player *>(
            world.GetEntityByID(playerID));
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
}

int main()
{
    TestContext test;

    {
        HealthPool health(25);

        test.ExpectEqual(
            health.GetCurrentHealth(),
            25,
            "HealthPool starts at full health");
        test.ExpectEqual(
            health.GetMaximumHealth(),
            25,
            "HealthPool stores its initial maximum health");
        test.Expect(
            health.IsAlive(),
            "A full HealthPool starts alive");
    }

    {
        HealthPool health(0);

        test.ExpectEqual(
            health.GetMaximumHealth(),
            1,
            "HealthPool clamps maximum health to at least one");
        test.ExpectEqual(
            health.GetCurrentHealth(),
            1,
            "HealthPool starts full after maximum health clamping");
    }

    {
        HealthPool health(20);

        health.ApplyDamage(7);

        test.ExpectEqual(
            health.GetCurrentHealth(),
            13,
            "Damage reduces current health");

        health.ApplyDamage(50);

        test.ExpectEqual(
            health.GetCurrentHealth(),
            0,
            "Damage cannot reduce health below zero");
        test.Expect(
            !health.IsAlive(),
            "HealthPool is not alive at zero health");

        health.Heal(6);

        test.ExpectEqual(
            health.GetCurrentHealth(),
            6,
            "Healing increases current health");
        test.Expect(
            health.IsAlive(),
            "HealthPool becomes alive above zero health");

        health.Heal(50);

        test.ExpectEqual(
            health.GetCurrentHealth(),
            20,
            "Healing cannot exceed maximum health");
    }

    {
        HealthPool health(18);

        health.ApplyDamage(5);
        health.ApplyDamage(0);
        health.ApplyDamage(-4);
        health.Heal(0);
        health.Heal(-6);

        test.ExpectEqual(
            health.GetCurrentHealth(),
            13,
            "Zero or negative damage and healing do nothing");
    }

    {
        HealthPool health(20);

        health.ApplyDamage(8);
        health.SetMaximumHealth(30);

        test.ExpectEqual(
            health.GetMaximumHealth(),
            30,
            "Increasing maximum health updates the cap");
        test.ExpectEqual(
            health.GetCurrentHealth(),
            12,
            "Increasing maximum health does not heal current health");

        health.SetMaximumHealth(10);

        test.ExpectEqual(
            health.GetMaximumHealth(),
            10,
            "Decreasing maximum health updates the cap");
        test.ExpectEqual(
            health.GetCurrentHealth(),
            10,
            "Decreasing maximum health clamps current health");

        health.RestoreToFull();

        test.ExpectEqual(
            health.GetCurrentHealth(),
            10,
            "RestoreToFull refills current health to maximum");
    }

    {
        Player player(1001);

        test.ExpectEqual(
            player.GetCurrentHealth(),
            player.GetMaximumHealth(),
            "A new player starts at full health");
        test.ExpectEqual(
            player.GetMaximumHealth(),
            player.GetTotalStat(StatType::MAX_HEALTH),
            "Player maximum health matches total MAX_HEALTH stat");
        test.Expect(
            player.GetMaximumHealth() >= 1,
            "Player maximum health never drops below one");

        player.ApplyDamage(30);

        test.ExpectEqual(
            player.GetCurrentHealth(),
            70,
            "Player damage reduces current health");

        player.Heal(10);

        test.ExpectEqual(
            player.GetCurrentHealth(),
            80,
            "Player healing restores current health");

        player.Heal(1000);

        test.ExpectEqual(
            player.GetCurrentHealth(),
            player.GetMaximumHealth(),
            "Player healing cannot exceed maximum health");

        player.ApplyDamage(1000);

        test.ExpectEqual(
            player.GetCurrentHealth(),
            0,
            "Player damage cannot reduce health below zero");
        test.Expect(
            !player.IsAlive(),
            "Player is not alive at zero health");

        player.RestoreHealthToFull();

        test.ExpectEqual(
            player.GetCurrentHealth(),
            player.GetMaximumHealth(),
            "Player can restore health to full");
        test.Expect(
            player.IsAlive(),
            "Player is alive after restoring health above zero");
    }

    {
        Player player(1002);

        player.ApplyDamage(20);
        player.GetEquipment().Equip(
            EquipmentSlotType::SHIELD,
            ItemType::WOODEN_SHIELD);
        player.RefreshDerivedState();

        test.ExpectEqual(
            player.GetMaximumHealth(),
            105,
            "Refreshing after shield equipment applies the maximum health bonus");
        test.ExpectEqual(
            player.GetCurrentHealth(),
            80,
            "Increasing maximum health does not automatically heal the player");

        player.Heal(1000);

        test.ExpectEqual(
            player.GetCurrentHealth(),
            105,
            "Healing can fill the increased maximum health");

        player.GetEquipment().Unequip(
            EquipmentSlotType::SHIELD);
        player.RefreshDerivedState();

        test.ExpectEqual(
            player.GetMaximumHealth(),
            100,
            "Refreshing after shield removal restores the base maximum health");
        test.ExpectEqual(
            player.GetCurrentHealth(),
            100,
            "Reducing maximum health clamps current health to the new cap");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            player != nullptr,
            "World creates a player entity for health tests");

        player->ApplyDamage(20);
        player->GetInventory().AddItem(
            ItemType::WOODEN_SHIELD,
            1);

        int shieldSlot = FindInventorySlot(
            player->GetInventory(),
            ItemType::WOODEN_SHIELD);

        test.Expect(
            shieldSlot >= 0,
            "Wooden shield exists in inventory for equip sync tests");
        test.Expect(
            world.TryEquipInventoryItem(
                playerID,
                shieldSlot),
            "World equip succeeds for the Wooden shield");
        test.ExpectEqual(
            player->GetMaximumHealth(),
            105,
            "Successful world equip refreshes player maximum health");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            80,
            "Successful world equip does not heal the added maximum health");

        player->GetInventory().AddItem(
            ItemType::BRONZE_SWORD,
            1);

        int swordSlot = FindInventorySlot(
            player->GetInventory(),
            ItemType::BRONZE_SWORD);

        test.Expect(
            swordSlot >= 0,
            "Bronze sword exists in inventory for unrelated equip tests");
        test.Expect(
            world.TryEquipInventoryItem(
                playerID,
                swordSlot),
            "World equip succeeds for the Bronze sword");
        test.ExpectEqual(
            player->GetMaximumHealth(),
            105,
            "Unrelated weapon equipment does not change maximum health");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            80,
            "Unrelated weapon equipment does not change current health");

        test.Expect(
            !world.TryEquipInventoryItem(
                playerID,
                -1),
            "Invalid equip requests fail safely");
        test.ExpectEqual(
            player->GetMaximumHealth(),
            105,
            "Failed equip does not change maximum health");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            80,
            "Failed equip does not change current health");
    }

    {
        Player player(1003);
        CombatRatings baseRatings =
            player.GetCombatRatings();

        test.ExpectEqual(
            baseRatings.attackAccuracy,
            player.GetTotalStat(StatType::ATTACK_ACCURACY),
            "Base attack accuracy rating matches the total stat");
        test.ExpectEqual(
            baseRatings.meleeStrength,
            player.GetTotalStat(StatType::MELEE_STRENGTH),
            "Base melee strength rating matches the total stat");
        test.ExpectEqual(
            baseRatings.defence,
            player.GetTotalStat(StatType::DEFENCE),
            "Base defence rating matches the total stat");
        test.ExpectEqual(
            baseRatings.maximumHealth,
            player.GetTotalStat(StatType::MAX_HEALTH),
            "Base maximum health rating matches the total stat");

        player.GetEquipment().Equip(
            EquipmentSlotType::WEAPON,
            ItemType::BRONZE_SWORD);

        CombatRatings swordRatings =
            player.GetCombatRatings();

        test.ExpectEqual(
            swordRatings.attackAccuracy,
            baseRatings.attackAccuracy + 3,
            "Bronze sword increases attack accuracy rating by its existing bonus");
        test.ExpectEqual(
            swordRatings.meleeStrength,
            baseRatings.meleeStrength + 4,
            "Bronze sword increases melee strength rating by its existing bonus");
        test.ExpectEqual(
            swordRatings.defence,
            baseRatings.defence,
            "Bronze sword does not change defence rating");
        test.ExpectEqual(
            swordRatings.maximumHealth,
            baseRatings.maximumHealth,
            "Bronze sword does not change maximum health rating");

        player.GetEquipment().Equip(
            EquipmentSlotType::SHIELD,
            ItemType::WOODEN_SHIELD);
        player.RefreshDerivedState();

        CombatRatings shieldRatings =
            player.GetCombatRatings();

        test.ExpectEqual(
            shieldRatings.attackAccuracy,
            swordRatings.attackAccuracy,
            "Wooden shield does not change attack accuracy rating");
        test.ExpectEqual(
            shieldRatings.meleeStrength,
            swordRatings.meleeStrength,
            "Wooden shield does not change melee strength rating");
        test.ExpectEqual(
            shieldRatings.defence,
            baseRatings.defence + 3,
            "Wooden shield increases defence rating by its existing bonus");
        test.ExpectEqual(
            shieldRatings.maximumHealth,
            baseRatings.maximumHealth + 5,
            "Wooden shield increases maximum health rating by its existing bonus");

        player.GetSkills().AddXP(
            SkillType::ATTACK,
            5000);

        CombatRatings attackXpRatings =
            player.GetCombatRatings();

        test.Expect(
            attackXpRatings.attackAccuracy >
                shieldRatings.attackAccuracy,
            "Attack XP increases attack accuracy rating");
        test.Expect(
            attackXpRatings.meleeStrength >
                shieldRatings.meleeStrength,
            "Attack XP increases melee strength rating");
        test.ExpectEqual(
            attackXpRatings.defence,
            shieldRatings.defence,
            "Attack XP does not change defence rating");

        player.GetSkills().AddXP(
            SkillType::DEFENCE,
            5000);

        CombatRatings defenceXpRatings =
            player.GetCombatRatings();

        test.Expect(
            defenceXpRatings.defence >
                attackXpRatings.defence,
            "Defence XP increases defence rating");
        test.ExpectEqual(
            defenceXpRatings.maximumHealth,
            attackXpRatings.maximumHealth,
            "Defence XP does not change maximum health rating");

        player.GetEquipment().Unequip(
            EquipmentSlotType::WEAPON);
        player.GetEquipment().Unequip(
            EquipmentSlotType::SHIELD);
        player.RefreshDerivedState();

        CombatRatings removedEquipmentRatings =
            player.GetCombatRatings();

        test.ExpectEqual(
            removedEquipmentRatings.attackAccuracy,
            player.GetBaseStat(StatType::ATTACK_ACCURACY),
            "Removing equipment returns attack accuracy rating to its base value");
        test.ExpectEqual(
            removedEquipmentRatings.meleeStrength,
            player.GetBaseStat(StatType::MELEE_STRENGTH),
            "Removing equipment returns melee strength rating to its base value");
        test.ExpectEqual(
            removedEquipmentRatings.defence,
            player.GetBaseStat(StatType::DEFENCE),
            "Removing equipment returns defence rating to its base value");
        test.ExpectEqual(
            removedEquipmentRatings.maximumHealth,
            player.GetBaseStat(StatType::MAX_HEALTH),
            "Removing equipment returns maximum health rating to its base value");
    }

    return test.Finish();
}