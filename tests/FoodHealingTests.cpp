#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Action/ActionCancelReason.h"
#include "../src/Action/ActionType.h"
#include "../src/Content/ContentValidator.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Inventory/Inventory.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Item/FoodDefinition.h"
#include "../src/Item/ItemDatabase.h"
#include "../src/Item/ToolType.h"
#include "../src/Player/Player.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/Stats/StatBlock.h"
#include "../src/Skills/SkillType.h"
#include "../src/StatusEffect/StatusEffectDefinition.h"
#include "../src/StatusEffect/StatusEffectModifiers.h"
#include "../src/StatusEffect/StatusEffectType.h"
#include "../src/World/Object/Resource/ResourceNode.h"
#include "../src/World/World.h"

#include <limits>

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
        const auto &slots =
            inventory.GetSlots();

        for (int index = 0;
             index < static_cast<int>(slots.size());
             ++index)
        {
            if (!slots[index].IsEmpty() &&
                slots[index].GetItemType() == itemType)
            {
                return index;
            }
        }

        return -1;
    }

    int CountItemSlots(
        const Inventory &inventory,
        ItemType itemType)
    {
        int count = 0;

        for (const InventorySlot &slot :
             inventory.GetSlots())
        {
            if (!slot.IsEmpty() &&
                slot.GetItemType() == itemType)
            {
                count++;
            }
        }

        return count;
    }

    bool StartGathering(
        World &world,
        int playerID)
    {
        Player *player =
            GetPlayer(world, playerID);

        if (player == nullptr)
        {
            return false;
        }

        int axeSlot = FindInventorySlot(
            player->GetInventory(),
            ItemType::BRONZE_AXE);

        if (axeSlot < 0 ||
            !world.TryEquipInventoryItem(
                playerID,
                axeSlot))
        {
            return false;
        }

        player->GetPosition().SetPosition(4, 5);

        ResourceNode *resource =
            world.GetResourceAt(5, 5);

        if (resource == nullptr)
        {
            return false;
        }

        world.QueueResourceInteraction(
            playerID,
            resource->GetID());
        world.Update();

        const Action *action =
            world.GetActionForEntity(playerID);

        return action != nullptr &&
               action->GetType() ==
                   ActionType::GATHERING;
    }

    bool StartMelee(
        World &world,
        int attackerID,
        int defenderID)
    {
        Player *attacker =
            GetPlayer(world, attackerID);
        Player *defender =
            GetPlayer(world, defenderID);

        if (attacker == nullptr ||
            defender == nullptr)
        {
            return false;
        }

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        if (!world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                4))
        {
            return false;
        }

        const Action *action =
            world.GetActionForEntity(attackerID);

        return action != nullptr &&
               action->GetType() ==
                   ActionType::MELEE_ATTACK;
    }

    bool SameInventory(
        const Inventory &left,
        const Inventory &right)
    {
        const auto &leftSlots = left.GetSlots();
        const auto &rightSlots = right.GetSlots();

        for (int index = 0;
             index < Inventory::SlotCount;
             ++index)
        {
            if (leftSlots[index].IsEmpty() != rightSlots[index].IsEmpty())
            {
                return false;
            }

            if (leftSlots[index].GetItemType() != rightSlots[index].GetItemType())
            {
                return false;
            }

            if (leftSlots[index].GetAmount() != rightSlots[index].GetAmount())
            {
                return false;
            }
        }

        return true;
    }
}

int main()
{
    TestContext test;

    {
        const ItemDefinition &cookedMeat =
            ItemDatabase::Get(
                ItemType::COOKED_MEAT);

        test.ExpectEqual(
            static_cast<int>(cookedMeat.GetItemType()),
            static_cast<int>(ItemType::COOKED_MEAT),
            "Cooked meat is registered");
        test.Expect(
            cookedMeat.IsFood(),
            "Cooked meat has a food definition");
        test.Expect(
            cookedMeat.GetFoodDefinition() != nullptr,
            "Cooked meat food definition is readable");
        test.ExpectEqual(
            cookedMeat.GetFoodDefinition()->healAmount,
            5,
            "Cooked meat heal amount is five");
        test.Expect(
            !cookedMeat.IsEquippable(),
            "Cooked meat is not equippable");
        test.ExpectEqual(
            static_cast<int>(cookedMeat.GetToolType()),
            static_cast<int>(ToolType::NONE),
            "Cooked meat is not a tool");

        bool foundCookedMeat = false;

        for (ItemType itemType : ItemDatabase::GetAllItemTypes())
        {
            if (itemType == ItemType::COOKED_MEAT)
            {
                foundCookedMeat = true;
                break;
            }
        }

        test.Expect(
            foundCookedMeat,
            "Cooked meat appears in item enumeration");
        test.Expect(
            ContentValidator::ValidateAll().IsValid(),
            "Current content validates with food item");

        ItemDefinition invalidFood(
            ItemType::COOKED_MEAT,
            "Bad food",
            false,
            EquipmentSlotType::NONE,
            ToolType::NONE,
            SkillType::NONE,
            0,
            0,
            StatBlock(),
            FoodDefinition{0});

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::COOKED_MEAT,
                 invalidFood)
                 .IsValid(),
            "Malformed food definition is rejected");
    }

    {
        Player player(7001);

        player.ApplyDamage(20);
        test.Expect(
            player.TryHeal(5),
            "Positive healing increases current health");
        test.ExpectEqual(
            player.GetCurrentHealth(),
            85,
            "Healing applies expected amount");

        player.TryHeal(1000);
        test.ExpectEqual(
            player.GetCurrentHealth(),
            player.GetMaximumHealth(),
            "Healing clamps to maximum health");

        int fullHealth =
            player.GetCurrentHealth();

        test.Expect(
            !player.TryHeal(1),
            "Full health cannot be healed");
        test.ExpectEqual(
            player.GetCurrentHealth(),
            fullHealth,
            "Full health remains unchanged");

        test.Expect(
            !player.TryHeal(0),
            "Zero healing fails safely");
        test.Expect(
            !player.TryHeal(-3),
            "Negative healing fails safely");

        player.ApplyDamage(1000);

        test.Expect(
            !player.TryHeal(10),
            "Dead player cannot heal");
        test.ExpectEqual(
            player.GetCurrentHealth(),
            0,
            "Dead player health remains zero");

        const int maxBefore =
            player.GetMaximumHealth();
        player.RestoreHealthToFull();
        player.ApplyDamage(5);
        player.TryHeal(2);

        test.ExpectEqual(
            player.GetMaximumHealth(),
            maxBefore,
            "Healing does not change maximum health");

        StatusEffectDefinition boost{
            StatusEffectType::MAX_HEALTH_BOOST,
            10,
            StatusEffectModifiers{0, 0, 0, 20}};

        World world;
        int playerID = world.CreatePlayer();
        Player *worldPlayer = GetPlayer(
            world,
            playerID);

        test.Expect(
            worldPlayer != nullptr,
            "World player exists for derived max-health test");

        worldPlayer->ApplyDamage(2);

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                boost),
            "Maximum-health boost applies");

        const int boostedMaximum =
            worldPlayer->GetMaximumHealth();

        test.Expect(
            boostedMaximum > 100,
            "Boosted maximum health is above base maximum");

        const int boostedCurrentBefore =
            worldPlayer->GetCurrentHealth();

        test.Expect(
            worldPlayer->TryHeal(std::numeric_limits<int>::max()),
            "Large healing request succeeds safely");
        test.ExpectEqual(
            worldPlayer->GetCurrentHealth(),
            boostedMaximum,
            "Healing uses derived maximum health and clamps safely");
        test.Expect(
            worldPlayer->GetCurrentHealth() >=
                boostedCurrentBefore,
            "Healing never decreases health");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            player != nullptr,
            "Player exists for world consume tests");

        test.Expect(
            !world.TryConsumeFood(
                -1,
                0),
            "Invalid player ID fails");

        int monsterID = world.CreateMonster(
            1,
            1,
            CombatRatings{1, 1, 1, 10});

        test.Expect(
            !world.TryConsumeFood(
                monsterID,
                0),
            "Non-player entity ID fails");

        test.Expect(
            !world.TryConsumeFood(
                playerID,
                -1),
            "Invalid inventory slot fails");

        int emptySlot = 0;
        const auto &slots =
            player->GetInventory().GetSlots();

        for (int index = 0;
             index < Inventory::SlotCount;
             ++index)
        {
            if (slots[index].IsEmpty())
            {
                emptySlot = index;
                break;
            }
        }

        test.Expect(
            !world.TryConsumeFood(
                playerID,
                emptySlot),
            "Empty slot fails");

        int nonFoodSlot =
            FindInventorySlot(
                player->GetInventory(),
                ItemType::BRONZE_AXE);

        test.Expect(
            nonFoodSlot >= 0,
            "Non-food item exists in inventory");
        test.Expect(
            !world.TryConsumeFood(
                playerID,
                nonFoodSlot),
            "Non-food item fails consumption");

        player->GetInventory().AddItem(
            ItemType::COOKED_MEAT,
            1);

        int foodSlot =
            FindInventorySlot(
                player->GetInventory(),
                ItemType::COOKED_MEAT);

        test.Expect(
            foodSlot >= 0,
            "Cooked meat exists in inventory");

        const int fullHealth =
            player->GetCurrentHealth();

        test.Expect(
            !world.TryConsumeFood(
                playerID,
                foodSlot),
            "Full-health player cannot consume food");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            fullHealth,
            "Full-health failure keeps health unchanged");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COOKED_MEAT),
            1,
            "Full-health failure preserves food");

        player->ApplyDamage(1000);
        test.Expect(
            !world.TryConsumeFood(
                playerID,
                foodSlot),
            "Dead player cannot consume food");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COOKED_MEAT),
            1,
            "Dead-player failure preserves food");

        player->RestoreHealthToFull();
        player->ApplyDamage(10);

        const int healthBeforeSuccess =
            player->GetCurrentHealth();

        test.Expect(
            world.TryConsumeFood(
                playerID,
                foodSlot),
            "Successful consumption works");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            healthBeforeSuccess + 5,
            "Successful consumption heals configured amount");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COOKED_MEAT),
            0,
            "Successful consumption removes exactly one item");

        player->GetInventory().AddItem(
            ItemType::COOKED_MEAT,
            1);

        int capSlot =
            FindInventorySlot(
                player->GetInventory(),
                ItemType::COOKED_MEAT);

        player->RestoreHealthToFull();
        player->ApplyDamage(2);

        test.Expect(
            world.TryConsumeFood(
                playerID,
                capSlot),
            "Consumption succeeds when below maximum");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            player->GetMaximumHealth(),
            "Healing caps at maximum health");

        player->ApplyDamage(15);
        player->GetInventory().AddItem(
            ItemType::COOKED_MEAT,
            2);

        int firstFoodSlot = -1;
        int secondFoodSlot = -1;

        const auto &duplicateSlots =
            player->GetInventory().GetSlots();

        for (int index = 0;
             index < Inventory::SlotCount;
             ++index)
        {
            if (duplicateSlots[index].IsEmpty() ||
                duplicateSlots[index].GetItemType() != ItemType::COOKED_MEAT)
            {
                continue;
            }

            if (firstFoodSlot < 0)
            {
                firstFoodSlot = index;
                continue;
            }

            secondFoodSlot = index;
            break;
        }

        test.Expect(
            firstFoodSlot >= 0 &&
                secondFoodSlot >= 0,
            "Duplicate non-stackable food occupies separate slots");

        const Inventory beforeDuplicateConsume =
            player->GetInventory();

        test.Expect(
            world.TryConsumeFood(
                playerID,
                secondFoodSlot),
            "Selected food slot can be consumed");

        const auto &afterDuplicateSlots =
            player->GetInventory().GetSlots();

        test.Expect(
            afterDuplicateSlots[secondFoodSlot].IsEmpty(),
            "Selected slot is the one consumed");
        test.Expect(
            !afterDuplicateSlots[firstFoodSlot].IsEmpty() &&
                afterDuplicateSlots[firstFoodSlot].GetItemType() == ItemType::COOKED_MEAT,
            "Another duplicate food slot remains unchanged");
        test.ExpectEqual(
            CountItemSlots(
                player->GetInventory(),
                ItemType::COOKED_MEAT),
            CountItemSlots(
                beforeDuplicateConsume,
                ItemType::COOKED_MEAT) -
                1,
            "Exactly one duplicate food item is removed");

        Inventory beforeFailInventory =
            player->GetInventory();
        int beforeFailHealth =
            player->GetCurrentHealth();

        test.Expect(
            !world.TryConsumeFood(
                playerID,
                nonFoodSlot),
            "Failed non-food consumption returns false");
        test.Expect(
            SameInventory(
                player->GetInventory(),
                beforeFailInventory),
            "Failed consumption leaves inventory unchanged");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            beforeFailHealth,
            "Failed consumption leaves health unchanged");

        const int maximumBeforeConsume =
            player->GetMaximumHealth();

        player->GetInventory().AddItem(
            ItemType::COOKED_MEAT,
            1);

        int repeatSlot =
            FindInventorySlot(
                player->GetInventory(),
                ItemType::COOKED_MEAT);

        test.Expect(
            world.TryConsumeFood(
                playerID,
                repeatSlot),
            "First repeat consumption succeeds");
        test.Expect(
            !world.TryConsumeFood(
                playerID,
                repeatSlot),
            "Repeated consumption requires another item");
        test.ExpectEqual(
            player->GetMaximumHealth(),
            maximumBeforeConsume,
            "Consumption does not change maximum health");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            player != nullptr,
            "Player exists for status-effect and action-policy tests");

        player->ApplyDamage(95);
        player->GetInventory().AddItem(
            ItemType::COOKED_MEAT,
            1);

        int buffFoodSlot =
            FindInventorySlot(
                player->GetInventory(),
                ItemType::COOKED_MEAT);

        StatusEffectDefinition maxHealthBuff{
            StatusEffectType::MAX_HEALTH_BOOST,
            20,
            StatusEffectModifiers{0, 0, 0, 20}};

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                maxHealthBuff),
            "Maximum-health buff applies");

        test.ExpectEqual(
            player->GetMaximumHealth(),
            120,
            "Buff increases maximum health ceiling");

        test.Expect(
            world.TryConsumeFood(
                playerID,
                buffFoodSlot),
            "Food can be consumed while buffed");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            10,
            "Healing uses buffed maximum-health context");

        player->GetInventory().AddItem(
            ItemType::COOKED_MEAT,
            1);

        int debuffFoodSlot =
            FindInventorySlot(
                player->GetInventory(),
                ItemType::COOKED_MEAT);

        StatusEffectDefinition maxHealthDebuff{
            StatusEffectType::COMBAT_REDUCTION,
            20,
            StatusEffectModifiers{0, 0, 0, -110}};

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                maxHealthDebuff),
            "Maximum-health debuff applies");

        test.ExpectEqual(
            player->GetMaximumHealth(),
            10,
            "Debuff lowers maximum health ceiling");

        player->ApplyDamage(3);

        test.Expect(
            world.TryConsumeFood(
                playerID,
                debuffFoodSlot),
            "Food can be consumed while debuffed");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            10,
            "Healing clamps to debuffed maximum health");

        player->ApplyDamage(4);
        player->GetInventory().AddItem(
            ItemType::COOKED_MEAT,
            1);

        int effectFoodSlot =
            FindInventorySlot(
                player->GetInventory(),
                ItemType::COOKED_MEAT);

        const auto effectsBefore =
            player->GetStatusEffectManager().GetActiveEffects();

        test.Expect(
            world.TryConsumeFood(
                playerID,
                effectFoodSlot),
            "Food consumption succeeds with active status effects");

        const auto effectsAfter =
            player->GetStatusEffectManager().GetActiveEffects();

        test.ExpectEqual(
            static_cast<int>(effectsAfter.size()),
            static_cast<int>(effectsBefore.size()),
            "Food consumption does not add or remove status effects");

        bool sameEffects = true;

        for (int index = 0;
             index < static_cast<int>(effectsBefore.size()) &&
             index < static_cast<int>(effectsAfter.size());
             ++index)
        {
            if (effectsBefore[index].type != effectsAfter[index].type ||
                effectsBefore[index].remainingTicks != effectsAfter[index].remainingTicks ||
                effectsBefore[index].lastAppliedTick != effectsAfter[index].lastAppliedTick)
            {
                sameEffects = false;
                break;
            }
        }

        test.Expect(
            sameEffects,
            "Food consumption does not refresh or alter status-effect timing");

        player->GetInventory().AddItem(
            ItemType::WOODEN_SHIELD,
            1);
        int shieldSlot =
            FindInventorySlot(
                player->GetInventory(),
                ItemType::WOODEN_SHIELD);

        test.Expect(
            world.TryEquipInventoryItem(
                playerID,
                shieldSlot),
            "Shield equip succeeds for equipment invariance test");

        ItemType equippedBefore =
            player->GetEquipment().GetEquippedItem(
                EquipmentSlotType::SHIELD);

        player->ApplyDamage(2);
        player->GetInventory().AddItem(
            ItemType::COOKED_MEAT,
            1);

        int equipmentFoodSlot =
            FindInventorySlot(
                player->GetInventory(),
                ItemType::COOKED_MEAT);

        test.Expect(
            world.TryConsumeFood(
                playerID,
                equipmentFoodSlot),
            "Food consumption succeeds with equipment equipped");
        test.ExpectEqual(
            static_cast<int>(
                player->GetEquipment().GetEquippedItem(
                    EquipmentSlotType::SHIELD)),
            static_cast<int>(equippedBefore),
            "Food consumption does not change equipment");

        test.Expect(
            StartGathering(
                world,
                playerID),
            "Gathering action starts for action-cancel policy test");

        player->ApplyDamage(4);
        player->GetInventory().AddItem(
            ItemType::COOKED_MEAT,
            1);

        int gatheringFoodSlot =
            FindInventorySlot(
                player->GetInventory(),
                ItemType::COOKED_MEAT);

        test.Expect(
            world.TryConsumeFood(
                playerID,
                gatheringFoodSlot),
            "Food can be consumed while gathering");

        const Action *gatheringAction =
            world.GetActionForEntity(
                playerID);

        test.Expect(
            gatheringAction != nullptr &&
                gatheringAction->GetType() == ActionType::GATHERING,
            "Consuming food does not cancel gathering");

        world.CancelActionsForEntity(
            playerID,
            ActionCancelReason::NONE);

        int defenderID =
            world.CreatePlayer();

        test.Expect(
            StartMelee(
                world,
                playerID,
                defenderID),
            "Melee action starts for action-cancel policy test");

        player->ApplyDamage(4);
        player->GetInventory().AddItem(
            ItemType::COOKED_MEAT,
            1);

        int meleeFoodSlot =
            FindInventorySlot(
                player->GetInventory(),
                ItemType::COOKED_MEAT);

        test.Expect(
            world.TryConsumeFood(
                playerID,
                meleeFoodSlot),
            "Food can be consumed while melee action is active");

        const Action *meleeAction =
            world.GetActionForEntity(
                playerID);

        test.Expect(
            meleeAction != nullptr &&
                meleeAction->GetType() == ActionType::MELEE_ATTACK,
            "Consuming food does not cancel melee");
    }

    return test.Finish();
}
