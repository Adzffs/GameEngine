#include "TestSupport.h"

#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Inventory/Inventory.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Item/ItemDatabase.h"
#include "../src/Player/Player.h"
#include "../src/Recipe/RecipeDatabase.h"
#include "../src/Recipe/RecipeSystem.h"
#include "../src/Recipe/RecipeType.h"
#include "../src/Requirement/Requirement.h"
#include "../src/Requirement/RequirementEvaluator.h"
#include "../src/Skills/SkillType.h"
#include "../src/World/Object/Station/CraftingStation.h"
#include "../src/World/World.h"

#include <initializer_list>
#include <string>
#include <vector>

namespace
{
    Player *GetPlayer(
        World &world,
        int playerID)
    {
        return dynamic_cast<Player *>(world.GetEntityByID(playerID));
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

    void OpenFurnace(
        TestContext &test,
        World &world,
        int playerID)
    {
        CraftingStation *station = world.GetStationAt(14, 9);

        test.Expect(
            station != nullptr,
            "Furnace exists for requirement tests");

        if (station == nullptr)
        {
            return;
        }

        world.QueueStationInteraction(
            playerID,
            station->GetID());
        world.Update();
    }

    void EquipItemThroughWorld(
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
            return;
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
            return;
        }

        test.Expect(
            world.TryEquipInventoryItem(
                playerID,
                slotIndex),
            "Item equips for requirement test setup");
    }

    RequirementSystem::Requirement SkillRequirement(
        SkillType skillType,
        int level)
    {
        return RequirementSystem::Requirement(
            RequirementSystem::SkillLevelRequirement{
                skillType,
                level});
    }

    RequirementSystem::Requirement HeldRequirement(
        ItemType itemType,
        int quantity)
    {
        return RequirementSystem::Requirement(
            RequirementSystem::HeldItemRequirement{
                itemType,
                quantity});
    }

    RequirementSystem::Requirement EquippedRequirement(
        ItemType itemType)
    {
        return RequirementSystem::Requirement(
            RequirementSystem::EquippedItemRequirement{
                itemType});
    }

    std::vector<RequirementSystem::Requirement> MakeRequirements(
        std::initializer_list<RequirementSystem::Requirement> requirements)
    {
        return std::vector<RequirementSystem::Requirement>(requirements);
    }
}

int main()
{
    TestContext test;

    {
        Player player(1);
        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            std::vector<RequirementSystem::Requirement>{});

        test.Expect(
            result.satisfied,
            "Empty requirement list succeeds");
    }

    {
        Player player(2);
        int woodcuttingLevel =
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetLevel();

        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({SkillRequirement(
                SkillType::WOODCUTTING,
                woodcuttingLevel)}));

        test.Expect(
            result.satisfied,
            "Skill exactly at requirement succeeds");
    }

    {
        Player player(3);
        int woodcuttingLevel =
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetLevel();

        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({SkillRequirement(
                SkillType::WOODCUTTING,
                woodcuttingLevel + 1)}));

        test.Expect(
            !result.satisfied,
            "Skill below requirement fails");
        test.ExpectEqual(
            result.message,
            std::string("You need Woodcutting level ") +
                std::to_string(woodcuttingLevel + 1),
            "Skill failure message uses the shared evaluator");
    }

    {
        Player player(4);
        player.GetInventory().AddItem(ItemType::COINS, 2);

        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({HeldRequirement(ItemType::COINS, 2)}));

        test.Expect(
            result.satisfied,
            "Held item exact quantity succeeds");
    }

    {
        Player player(5);
        player.GetInventory().AddItem(ItemType::COINS, 1);

        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({HeldRequirement(ItemType::COINS, 2)}));

        test.Expect(
            !result.satisfied,
            "Insufficient held quantity fails");
        test.ExpectEqual(
            result.message,
            std::string("You need 2 Coins"),
            "Held item failure message uses the shared evaluator");
    }

    {
        Player player(6);
        player.GetEquipment().Equip(
            EquipmentSlotType::WEAPON,
            ItemType::BRONZE_AXE);

        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({EquippedRequirement(ItemType::BRONZE_AXE)}));

        test.Expect(
            result.satisfied,
            "Equipped item succeeds");
    }

    {
        Player player(7);
        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({EquippedRequirement(ItemType::BRONZE_AXE)}));

        test.Expect(
            !result.satisfied,
            "Missing equipped item fails");
        test.ExpectEqual(
            result.message,
            std::string("You need to equip Bronze axe"),
            "Equipped item failure message uses the shared evaluator");
    }

    {
        Player player(8);
        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({SkillRequirement(
                static_cast<SkillType>(999),
                1)}));

        test.Expect(
            !result.satisfied,
            "Invalid skill type fails safely");
    }

    {
        Player player(9);
        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({SkillRequirement(
                SkillType::WOODCUTTING,
                0)}));

        test.Expect(
            !result.satisfied,
            "Non-positive level fails safely");
    }

    {
        Player player(10);
        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({HeldRequirement(
                static_cast<ItemType>(999),
                1)}));

        test.Expect(
            !result.satisfied,
            "Invalid item ID fails safely");
    }

    {
        Player player(11);
        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({HeldRequirement(ItemType::COAL, 0)}));

        test.Expect(
            !result.satisfied,
            "Non-positive held quantity fails safely");
    }

    {
        Player player(12);
        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({HeldRequirement(ItemType::COAL, -1)}));

        test.Expect(
            !result.satisfied,
            "Negative held quantity fails safely");
    }

    {
        Player player(13);
        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({EquippedRequirement(
                static_cast<ItemType>(999))}));

        test.Expect(
            !result.satisfied,
            "Invalid equipped item ID fails safely");
    }

    {
        Player player(14);
        player.GetEquipment().Equip(
            EquipmentSlotType::SHIELD,
            ItemType::WOODEN_SHIELD);

        ItemType shieldBefore =
            player.GetEquipment().GetEquippedItem(
                EquipmentSlotType::SHIELD);

        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({EquippedRequirement(ItemType::WOODEN_SHIELD)}));

        test.Expect(
            result.satisfied,
            "Equipped item requirement finds an item in a non-weapon slot");
        test.ExpectEqual(
            static_cast<int>(
                player.GetEquipment().GetEquippedItem(
                    EquipmentSlotType::SHIELD)),
            static_cast<int>(shieldBefore),
            "Equipped item evaluation does not mutate non-weapon equipment");
    }

    {
        Player player(15);
        int woodcuttingLevel =
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetLevel();

        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({SkillRequirement(
                                  SkillType::WOODCUTTING,
                                  woodcuttingLevel + 1),
                              HeldRequirement(
                                  ItemType::COAL,
                                  1)}));

        test.Expect(
            !result.satisfied,
            "First failed requirement determines the result");
        test.ExpectEqual(
            result.message,
            std::string("You need Woodcutting level ") +
                std::to_string(woodcuttingLevel + 1),
            "First failed requirement returns its message");
    }

    {
        Player player(13);
        player.GetInventory().AddItem(ItemType::COINS, 2);
        player.GetEquipment().Equip(
            EquipmentSlotType::WEAPON,
            ItemType::BRONZE_AXE);
        player.ApplyDamage(1);

        int woodcuttingLevel =
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetLevel();
        int coinsBefore =
            player.GetInventory().GetItemAmount(ItemType::COINS);
        ItemType equippedBefore =
            player.GetEquipment().GetEquippedItem(
                EquipmentSlotType::WEAPON);
        int woodcuttingLevelBefore =
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetLevel();
        int woodcuttingXPBefore =
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetXP();
        int currentHealthBefore = player.GetCurrentHealth();
        int maximumHealthBefore = player.GetMaximumHealth();

        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({SkillRequirement(
                                  SkillType::WOODCUTTING,
                                  woodcuttingLevel + 1),
                              HeldRequirement(
                                  ItemType::COAL,
                                  1)}));

        test.Expect(
            !result.satisfied,
            "Evaluation can fail without mutating player state");
        test.ExpectEqual(
            player.GetInventory().GetItemAmount(ItemType::COINS),
            coinsBefore,
            "Evaluation does not mutate inventory");
        test.ExpectEqual(
            static_cast<int>(
                player.GetEquipment().GetEquippedItem(
                    EquipmentSlotType::WEAPON)),
            static_cast<int>(equippedBefore),
            "Evaluation does not mutate equipment");
        test.ExpectEqual(
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetLevel(),
            woodcuttingLevelBefore,
            "Evaluation does not mutate skill levels");
        test.ExpectEqual(
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetXP(),
            woodcuttingXPBefore,
            "Evaluation does not mutate skill XP");
        test.ExpectEqual(
            player.GetCurrentHealth(),
            currentHealthBefore,
            "Evaluation does not mutate current health");
        test.ExpectEqual(
            player.GetMaximumHealth(),
            maximumHealthBefore,
            "Evaluation does not mutate maximum health");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            player != nullptr,
            "Player exists for recipe integration test");

        if (player != nullptr)
        {
            player->GetPosition().SetPosition(13, 9);
            OpenFurnace(
                test,
                world,
                playerID);

            int smithingLevel =
                player->GetSkills().GetSkill(SkillType::SMITHING).GetLevel();
            int requiredLevel =
                RecipeDatabase::Get(RecipeType::STEEL_BAR)
                    .GetRequiredLevel();

            player->GetInventory().AddItem(
                ItemType::IRON_ORE,
                1);
            player->GetInventory().AddItem(
                ItemType::COAL,
                2);

            bool shouldFail = smithingLevel < requiredLevel;

            test.Expect(
                shouldFail == !world.TryStartRecipeAction(
                                  playerID,
                                  RecipeType::STEEL_BAR),
                "Recipe start matches the shared requirement result");

            if (shouldFail)
            {
                player->GetSkills().AddXP(
                    SkillType::SMITHING,
                    100000);

                test.Expect(
                    world.TryStartRecipeAction(
                        playerID,
                        RecipeType::STEEL_BAR),
                    "Recipe starts after the shared requirement is met");
            }

            AdvanceWorldTicks(
                world,
                RecipeDatabase::Get(RecipeType::STEEL_BAR)
                    .GetActionDurationTicks());

            test.ExpectEqual(
                player->GetInventory().GetItemAmount(ItemType::STEEL_BAR),
                1,
                "Recipe completion still creates the expected output");
        }
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            player != nullptr,
            "Player exists for equipment requirement test");

        if (player != nullptr)
        {
            player->GetInventory().AddItem(
                ItemType::STEEL_AXE,
                1);

            int steelAxeSlot = FindInventorySlot(
                player->GetInventory(),
                ItemType::STEEL_AXE);

            test.Expect(
                steelAxeSlot >= 0,
                "Steel axe exists in inventory for equipment requirement test");

            if (steelAxeSlot >= 0)
            {
                int woodcuttingLevel =
                    player->GetSkills().GetSkill(SkillType::WOODCUTTING).GetLevel();
                int requiredLevel =
                    ItemDatabase::Get(ItemType::STEEL_AXE)
                        .GetRequiredSkillLevel();

                bool shouldFail = woodcuttingLevel < requiredLevel;

                test.Expect(
                    shouldFail == !world.TryEquipInventoryItem(
                                      playerID,
                                      steelAxeSlot),
                    "Equipment requirement result matches the shared evaluator");

                if (shouldFail)
                {
                    player->GetSkills().AddXP(
                        SkillType::WOODCUTTING,
                        100000);

                    player->GetInventory().AddItem(
                        ItemType::STEEL_AXE,
                        1);

                    steelAxeSlot = FindInventorySlot(
                        player->GetInventory(),
                        ItemType::STEEL_AXE);

                    test.Expect(
                        steelAxeSlot >= 0,
                        "Steel axe exists again after the failed equip attempt");

                    test.Expect(
                        world.TryEquipInventoryItem(
                            playerID,
                            steelAxeSlot),
                        "Equipment requirement succeeds after the shared requirement is met");
                }

                test.ExpectEqual(
                    static_cast<int>(
                        player->GetEquipment().GetEquippedItem(
                            EquipmentSlotType::WEAPON)),
                    static_cast<int>(ItemType::STEEL_AXE),
                    "Equipment requirement equips the expected weapon");
            }
        }
    }

    {
        Player player(14);
        player.GetInventory().AddItem(ItemType::COINS, 2);
        player.GetEquipment().Equip(
            EquipmentSlotType::WEAPON,
            ItemType::BRONZE_AXE);
        player.ApplyDamage(1);

        int woodcuttingLevel =
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetLevel();
        int coinsBefore =
            player.GetInventory().GetItemAmount(ItemType::COINS);
        ItemType equippedBefore =
            player.GetEquipment().GetEquippedItem(
                EquipmentSlotType::WEAPON);
        int woodcuttingLevelBefore =
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetLevel();
        int woodcuttingXPBefore =
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetXP();
        int currentHealthBefore = player.GetCurrentHealth();
        int maximumHealthBefore = player.GetMaximumHealth();

        auto result = RequirementSystem::RequirementEvaluator::EvaluateAll(
            player,
            MakeRequirements({SkillRequirement(
                                  SkillType::WOODCUTTING,
                                  woodcuttingLevel + 1),
                              HeldRequirement(
                                  ItemType::COAL,
                                  1)}));

        test.Expect(
            !result.satisfied,
            "Evaluation can fail without mutating player state");
        test.ExpectEqual(
            player.GetInventory().GetItemAmount(ItemType::COINS),
            coinsBefore,
            "Evaluation does not mutate inventory");
        test.ExpectEqual(
            static_cast<int>(
                player.GetEquipment().GetEquippedItem(
                    EquipmentSlotType::WEAPON)),
            static_cast<int>(equippedBefore),
            "Evaluation does not mutate equipment");
        test.ExpectEqual(
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetLevel(),
            woodcuttingLevelBefore,
            "Evaluation does not mutate skill levels");
        test.ExpectEqual(
            player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetXP(),
            woodcuttingXPBefore,
            "Evaluation does not mutate skill XP");
        test.ExpectEqual(
            player.GetCurrentHealth(),
            currentHealthBefore,
            "Evaluation does not mutate current health");
        test.ExpectEqual(
            player.GetMaximumHealth(),
            maximumHealthBefore,
            "Evaluation does not mutate maximum health");
    }

    return test.Finish();
}
