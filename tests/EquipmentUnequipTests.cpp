#include "TestSupport.h"

#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Inventory/Inventory.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Item/ItemDatabase.h"
#include "../src/Player/Player.h"
#include "../src/Skills/SkillType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/Stats/StatType.h"
#include "../src/World/World.h"

#include <string>

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

        for (int slotIndex = 0;
             slotIndex < static_cast<int>(slots.size());
             ++slotIndex)
        {
            if (!slots[slotIndex].IsEmpty() &&
                slots[slotIndex].GetItemType() == itemType)
            {
                return slotIndex;
            }
        }

        return -1;
    }

    bool EquipItem(
        TestContext &test,
        World &world,
        int playerID,
        ItemType itemType)
    {
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            player != nullptr,
            "Player exists for equip setup");

        if (player == nullptr)
        {
            return false;
        }

        player->GetInventory().AddItem(itemType, 1);

        int slotIndex = FindInventorySlot(
            player->GetInventory(),
            itemType);

        test.Expect(
            slotIndex >= 0,
            "Item exists in inventory for equip setup");

        if (slotIndex < 0)
        {
            return false;
        }

        return world.TryEquipInventoryItem(
            playerID,
            slotIndex);
    }

    void AdvanceWorldTicks(
        World &world,
        int ticks)
    {
        for (int tickIndex = 0; tickIndex < ticks; ++tickIndex)
        {
            world.Update();
        }
    }

    void FillInventoryWithBronzeSwords(
        Player &player)
    {
        while (player.GetInventory().AddItem(
            ItemType::BRONZE_SWORD,
            1))
        {
        }
    }
}

int main()
{
    TestContext test;

    {
        World world;
        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            2,
            2,
            CombatRatings{});

        test.Expect(
            !world.TryUnequipItem(
                -1,
                EquipmentSlotType::WEAPON),
            "Invalid player ID fails safely");
        test.Expect(
            !world.TryUnequipItem(
                monsterID,
                EquipmentSlotType::WEAPON),
            "Non-Player entity ID fails safely");
        test.Expect(
            !world.TryUnequipItem(
                playerID,
                EquipmentSlotType::NONE),
            "EquipmentSlotType::NONE fails");
        test.Expect(
            !world.TryUnequipItem(
                playerID,
                static_cast<EquipmentSlotType>(999)),
            "Unknown slot enum fails");
        test.Expect(
            !world.TryUnequipItem(
                playerID,
                EquipmentSlotType::WEAPON),
            "Empty slot fails");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            player != nullptr,
            "Player exists for weapon unequip test");

        if (player != nullptr)
        {
            test.Expect(
                EquipItem(
                    test,
                    world,
                    playerID,
                    ItemType::DEVELOPER_GODSWORD),
                "Developer Godsword equips for unequip test");

            int weaponSlotBefore =
                FindInventorySlot(
                    player->GetInventory(),
                    ItemType::DEVELOPER_GODSWORD);
            int attackBefore =
                player->GetEquipmentBonus(StatType::ATTACK_ACCURACY);

            test.Expect(
                weaponSlotBefore < 0,
                "Developer Godsword is removed from inventory when equipped");
            test.Expect(
                world.TryUnequipItem(
                    playerID,
                    EquipmentSlotType::WEAPON),
                "Weapon unequips successfully");
            test.ExpectEqual(
                player->GetInventory().GetItemAmount(
                    ItemType::DEVELOPER_GODSWORD),
                1,
                "Removed item enters inventory");
            test.ExpectEqual(
                static_cast<int>(
                    player->GetEquipment().GetEquippedItem(
                        EquipmentSlotType::WEAPON)),
                static_cast<int>(ItemType::NONE),
                "Equipment slot becomes empty");
            test.ExpectEqual(
                player->GetEquipmentBonus(StatType::ATTACK_ACCURACY),
                0,
                "Successful unequip recalculates derived stats");

            int reequippedSlot = FindInventorySlot(
                player->GetInventory(),
                ItemType::DEVELOPER_GODSWORD);

            test.Expect(
                reequippedSlot >= 0,
                "Unequipped weapon returns to inventory slot for re-equip");
            test.Expect(
                world.TryEquipInventoryItem(
                    playerID,
                    reequippedSlot),
                "Existing equip behaviour remains unchanged");
            test.Expect(
                player->GetEquipmentBonus(StatType::ATTACK_ACCURACY) >= attackBefore,
                "Weapon bonuses return after re-equipping");
        }
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            player != nullptr,
            "Player exists for shield unequip test");

        if (player != nullptr)
        {
            player->ApplyDamage(20);
            test.Expect(
                EquipItem(
                    test,
                    world,
                    playerID,
                    ItemType::BRONZE_SWORD),
                "Bronze sword equips for unrelated bonus test");
            test.Expect(
                EquipItem(
                    test,
                    world,
                    playerID,
                    ItemType::WOODEN_SHIELD),
                "Wooden shield equips for health clamp test");

            test.ExpectEqual(
                player->GetMaximumHealth(),
                105,
                "Shield increases maximum health");
            test.ExpectEqual(
                player->GetCurrentHealth(),
                80,
                "Shield does not automatically heal current health");

            player->Heal(1000);

            int attackBonusBefore =
                player->GetEquipmentBonus(
                    StatType::ATTACK_ACCURACY);

            test.Expect(
                world.TryUnequipItem(
                    playerID,
                    EquipmentSlotType::SHIELD),
                "Shield unequips successfully");
            test.ExpectEqual(
                player->GetInventory().GetItemAmount(
                    ItemType::WOODEN_SHIELD),
                1,
                "Removed shield enters inventory");
            test.ExpectEqual(
                static_cast<int>(
                    player->GetEquipment().GetEquippedItem(
                        EquipmentSlotType::SHIELD)),
                static_cast<int>(ItemType::NONE),
                "Shield slot becomes empty");
            test.ExpectEqual(
                player->GetMaximumHealth(),
                100,
                "Maximum health decreases correctly");
            test.ExpectEqual(
                player->GetCurrentHealth(),
                100,
                "Current health clamps to new maximum");
            test.ExpectEqual(
                player->GetEquipmentBonus(StatType::ATTACK_ACCURACY),
                attackBonusBefore,
                "Unrelated equipment bonuses remain");
        }
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            player != nullptr,
            "Player exists for full inventory failure test");

        if (player != nullptr)
        {
            test.Expect(
                EquipItem(
                    test,
                    world,
                    playerID,
                    ItemType::WOODEN_SHIELD),
                "Wooden shield equips for full inventory test");

            FillInventoryWithBronzeSwords(*player);

            int bronzeSwordCountBefore =
                player->GetInventory().GetItemAmount(
                    ItemType::BRONZE_SWORD);
            int shieldCountBefore =
                player->GetInventory().GetItemAmount(
                    ItemType::WOODEN_SHIELD);

            test.Expect(
                !world.TryUnequipItem(
                    playerID,
                    EquipmentSlotType::SHIELD),
                "Full inventory causes complete failure");
            test.ExpectEqual(
                static_cast<int>(
                    player->GetEquipment().GetEquippedItem(
                        EquipmentSlotType::SHIELD)),
                static_cast<int>(ItemType::WOODEN_SHIELD),
                "Full-inventory failure leaves equipment unchanged");
            test.ExpectEqual(
                player->GetInventory().GetItemAmount(
                    ItemType::BRONZE_SWORD),
                bronzeSwordCountBefore,
                "Full-inventory failure leaves inventory unchanged");
            test.ExpectEqual(
                player->GetInventory().GetItemAmount(
                    ItemType::WOODEN_SHIELD),
                shieldCountBefore,
                "Full inventory does not add the removed item");
        }
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            player != nullptr,
            "Player exists for gathering cancellation test");

        if (player != nullptr)
        {
            player->GetPosition().SetPosition(4, 5);
            test.Expect(
                EquipItem(
                    test,
                    world,
                    playerID,
                    ItemType::BRONZE_AXE),
                "Bronze axe equips for gathering cancellation test");

            ResourceNode *resource = world.GetResourceAt(5, 5);
            int woodcuttingXPBefore =
                player->GetSkills().GetSkill(
                                       SkillType::WOODCUTTING)
                    .GetXP();
            int logsBefore =
                player->GetInventory().GetItemAmount(ItemType::LOG);

            test.Expect(
                resource != nullptr,
                "Normal tree exists for gathering cancellation test");

            if (resource != nullptr)
            {
                world.QueueResourceInteraction(
                    playerID,
                    resource->GetID());
                world.Update();

                test.Expect(
                    world.GetActionForEntity(playerID) != nullptr,
                    "Gathering action starts before unequip cancellation");

                test.Expect(
                    world.TryUnequipItem(
                        playerID,
                        EquipmentSlotType::WEAPON),
                    "Unequipping an active gathering tool cancels the action");

                AdvanceWorldTicks(
                    world,
                    ItemDatabase::Get(ItemType::BRONZE_AXE)
                        .GetActionDurationTicks());

                test.ExpectEqual(
                    player->GetInventory().GetItemAmount(ItemType::LOG),
                    logsBefore,
                    "Cancelled gathering grants no reward");
                test.ExpectEqual(
                    player->GetSkills().GetSkill(
                                           SkillType::WOODCUTTING)
                        .GetXP(),
                    woodcuttingXPBefore,
                    "Cancelled gathering grants no XP");
                test.Expect(
                    world.GetActionForEntity(playerID) == nullptr,
                    "Cancelled gathering does not remain active");
            }
        }
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            player != nullptr,
            "Player exists for unrelated armour cancellation test");

        if (player != nullptr)
        {
            player->GetPosition().SetPosition(4, 5);
            test.Expect(
                EquipItem(
                    test,
                    world,
                    playerID,
                    ItemType::BRONZE_AXE),
                "Bronze axe equips for unrelated armour cancellation test");
            test.Expect(
                EquipItem(
                    test,
                    world,
                    playerID,
                    ItemType::WOODEN_SHIELD),
                "Wooden shield equips for unrelated armour cancellation test");

            ResourceNode *resource = world.GetResourceAt(5, 5);

            test.Expect(
                resource != nullptr,
                "Normal tree exists for unrelated armour cancellation test");

            if (resource != nullptr)
            {
                world.QueueResourceInteraction(
                    playerID,
                    resource->GetID());
                world.Update();

                test.Expect(
                    world.GetActionForEntity(playerID) != nullptr,
                    "Gathering action starts before unrelated unequip");

                test.Expect(
                    world.TryUnequipItem(
                        playerID,
                        EquipmentSlotType::SHIELD),
                    "Unequipping unrelated armour succeeds");
                test.Expect(
                    world.GetActionForEntity(playerID) != nullptr,
                    "Unequipping unrelated armour does not cancel gathering");
            }
        }
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            player != nullptr,
            "Player exists for populated slot coverage test");

        if (player != nullptr)
        {
            test.Expect(
                EquipItem(
                    test,
                    world,
                    playerID,
                    ItemType::DEVELOPER_GODSWORD),
                "Developer Godsword equips for populated slot coverage test");
            test.Expect(
                world.TryUnequipItem(
                    playerID,
                    EquipmentSlotType::WEAPON),
                "Generic method works for the weapon slot");

            test.Expect(
                EquipItem(
                    test,
                    world,
                    playerID,
                    ItemType::WOODEN_SHIELD),
                "Wooden shield equips for populated slot coverage test");
            test.Expect(
                world.TryUnequipItem(
                    playerID,
                    EquipmentSlotType::SHIELD),
                "Generic method works for the shield slot");
        }
    }

    return test.Finish();
}