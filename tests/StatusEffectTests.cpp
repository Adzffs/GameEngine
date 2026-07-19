#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Action/ActionType.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Player/Player.h"
#include "../src/Stats/StatType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/StatusEffect/StatusEffectDefinition.h"
#include "../src/StatusEffect/StatusEffectManager.h"
#include "../src/StatusEffect/StatusEffectType.h"
#include "../src/StatusEffect/StatusEffectValidation.h"
#include "../src/World/World.h"

#include <limits>
#include <type_traits>

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

    int FindInventorySlot(
        const Inventory &inventory,
        ItemType itemType)
    {
        const auto &slots = inventory.GetSlots();

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

    bool EquipItem(
        World &world,
        int playerID,
        ItemType itemType)
    {
        Player *player = GetPlayer(
            world,
            playerID);

        if (player == nullptr)
        {
            return false;
        }

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

    bool StartGatheringAction(
        World &world,
        int playerID)
    {
        Player *player = GetPlayer(
            world,
            playerID);

        if (player == nullptr)
        {
            return false;
        }

        if (!EquipItem(
                world,
                playerID,
                ItemType::BRONZE_AXE))
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

    bool StartMeleeAction(
        World &world,
        int attackerID,
        int defenderID,
        int durationTicks)
    {
        Player *attacker = GetPlayer(
            world,
            attackerID);
        Player *defender = GetPlayer(
            world,
            defenderID);

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
                durationTicks))
        {
            return false;
        }

        const Action *action =
            world.GetActionForEntity(attackerID);

        return action != nullptr &&
               action->GetType() ==
                   ActionType::MELEE_ATTACK;
    }

    StatusEffectDefinition MakeEffect(
        StatusEffectType type,
        int durationTicks,
        int accuracy,
        int strength,
        int defence,
        int maximumHealth)
    {
        return StatusEffectDefinition{
            type,
            durationTicks,
            StatusEffectModifiers{
                accuracy,
                strength,
                defence,
                maximumHealth}};
    }
}

int main()
{
    static_assert(
        std::is_same_v<
            decltype(std::declval<const StatusEffectManager &>()
                         .GetActiveEffects()),
            const std::vector<ActiveStatusEffect> &>,
        "GetActiveEffects must return a read-only collection");

    TestContext test;

    {
        StatusEffectManager manager;

        test.Expect(
            manager.GetActiveEffects().empty(),
            "StatusEffectManager starts empty");
        test.ExpectEqual(
            manager.GetCombinedModifiers().meleeAccuracy,
            0,
            "Empty manager has zero combined modifiers");
    }

    {
        StatusEffectManager manager;

        StatusEffectDefinition buff =
            MakeEffect(
                StatusEffectType::COMBAT_BOOST,
                3,
                5,
                7,
                0,
                0);

        StatusEffectDefinition debuff =
            MakeEffect(
                StatusEffectType::COMBAT_REDUCTION,
                2,
                -4,
                -3,
                -2,
                0);

        test.Expect(
            manager.Apply(buff),
            "Valid buff applies successfully");
        test.Expect(
            manager.Apply(debuff),
            "Valid debuff applies successfully");
        test.Expect(
            manager.HasEffect(
                StatusEffectType::COMBAT_BOOST),
            "Applied buff is discoverable");
        test.Expect(
            manager.HasEffect(
                StatusEffectType::COMBAT_REDUCTION),
            "Applied debuff is discoverable");
    }

    {
        StatusEffectManager manager;

        test.Expect(
            !manager.Apply(
                MakeEffect(
                    StatusEffectType::NONE,
                    2,
                    1,
                    0,
                    0,
                    0)),
            "NONE type definitions are rejected");
        test.Expect(
            !manager.Apply(
                MakeEffect(
                    static_cast<StatusEffectType>(999),
                    2,
                    1,
                    0,
                    0,
                    0)),
            "Unknown enum-cast types are rejected");
        test.Expect(
            !manager.Apply(
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    0,
                    1,
                    0,
                    0,
                    0)),
            "Zero duration definitions are rejected");
        test.Expect(
            !manager.Apply(
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    -1,
                    1,
                    0,
                    0,
                    0)),
            "Negative duration definitions are rejected");
        test.Expect(
            !manager.Apply(
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    2,
                    0,
                    0,
                    0,
                    0)),
            "All-zero modifiers are rejected");
    }

    {
        StatusEffectManager manager;

        test.Expect(
            manager.Apply(
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    5,
                    4,
                    0,
                    0,
                    0)),
            "First effect applies");
        test.Expect(
            manager.Apply(
                MakeEffect(
                    StatusEffectType::MAX_HEALTH_BOOST,
                    5,
                    0,
                    0,
                    0,
                    10)),
            "Different type applies");

        test.ExpectEqual(
            static_cast<int>(
                manager.GetActiveEffects().size()),
            2,
            "Different effect types stack instead of replacing");

        StatusEffectModifiers combined =
            manager.GetCombinedModifiers();

        test.ExpectEqual(
            combined.meleeAccuracy,
            4,
            "Combined modifiers add across active effects for accuracy");
        test.ExpectEqual(
            combined.maximumHealth,
            10,
            "Combined modifiers add across active effects for maximum health");
    }

    {
        StatusEffectManager manager;

        test.Expect(
            manager.Apply(
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    3,
                    2,
                    0,
                    0,
                    0)),
            "Initial same-type effect applies");
        test.Expect(
            manager.Apply(
                MakeEffect(
                    StatusEffectType::MAX_HEALTH_BOOST,
                    6,
                    0,
                    0,
                    0,
                    5)),
            "Second type establishes deterministic ordering");
        test.Expect(
            manager.Apply(
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    9,
                    9,
                    0,
                    0,
                    0)),
            "Reapplying same type succeeds by replacing existing entry");

        test.ExpectEqual(
            static_cast<int>(
                manager.GetActiveEffects().size()),
            2,
            "Same-type reapplication does not create duplicate stacks");

        const ActiveStatusEffect *reapplied =
            manager.FindEffect(
                StatusEffectType::COMBAT_BOOST);

        test.Expect(
            reapplied != nullptr,
            "Reapplied effect is still present");

        if (reapplied != nullptr)
        {
            test.ExpectEqual(
                reapplied->modifiers.meleeAccuracy,
                9,
                "Same-type reapplication replaces modifiers");
            test.ExpectEqual(
                reapplied->remainingTicks,
                9,
                "Same-type reapplication resets remaining duration");
        }

        const std::vector<ActiveStatusEffect> &ordered =
            manager.GetActiveEffects();

        test.Expect(
            ordered.size() == 2 &&
                ordered[0].type == StatusEffectType::COMBAT_BOOST &&
                ordered[1].type == StatusEffectType::MAX_HEALTH_BOOST,
            "Same-type reapplication preserves deterministic iteration order");
    }

    {
        StatusEffectManager manager;

        manager.Apply(
            MakeEffect(
                StatusEffectType::COMBAT_BOOST,
                2,
                1,
                0,
                0,
                0));

        test.Expect(
            manager.Remove(
                StatusEffectType::COMBAT_BOOST),
            "Removing an active effect succeeds");
        test.Expect(
            !manager.HasEffect(
                StatusEffectType::COMBAT_BOOST),
            "Removed effect no longer exists");

        std::vector<ActiveStatusEffect> before =
            manager.GetActiveEffects();

        test.Expect(
            !manager.Remove(
                StatusEffectType::MAX_HEALTH_BOOST),
            "Removing a missing effect fails");
        test.ExpectEqual(
            static_cast<int>(
                manager.GetActiveEffects().size()),
            static_cast<int>(before.size()),
            "Failed remove leaves effect collection unchanged");
    }

    {
        StatusEffectManager manager;

        manager.Apply(
            MakeEffect(
                StatusEffectType::COMBAT_BOOST,
                3,
                1,
                0,
                0,
                0));
        manager.Apply(
            MakeEffect(
                StatusEffectType::MAX_HEALTH_BOOST,
                2,
                0,
                0,
                0,
                5));

        test.Expect(
            !manager.Tick(),
            "Tick reports no expiry when all remaining durations stay above zero");

        const ActiveStatusEffect *combatBoost =
            manager.FindEffect(
                StatusEffectType::COMBAT_BOOST);
        const ActiveStatusEffect *healthBoost =
            manager.FindEffect(
                StatusEffectType::MAX_HEALTH_BOOST);

        test.Expect(
            combatBoost != nullptr &&
                combatBoost->remainingTicks == 2,
            "Tick decrements each active effect exactly once");
        test.Expect(
            healthBoost != nullptr &&
                healthBoost->remainingTicks == 1,
            "Tick decrements independent effects exactly once");

        test.Expect(
            manager.Tick(),
            "Tick reports expiry when duration reaches zero");

        test.Expect(
            !manager.HasEffect(
                StatusEffectType::MAX_HEALTH_BOOST),
            "Duration-one remaining effect expires after one completed tick");

        test.Expect(
            manager.HasEffect(
                StatusEffectType::COMBAT_BOOST),
            "Other active effects remain after deterministic expiry pass");

        manager.Apply(
            MakeEffect(
                StatusEffectType::COMBAT_REDUCTION,
                1,
                -2,
                0,
                0,
                0));

        manager.Apply(
            MakeEffect(
                StatusEffectType::MAX_HEALTH_BOOST,
                1,
                0,
                0,
                0,
                2));

        test.Expect(
            manager.Tick(),
            "Expiry pass is deterministic when multiple effects end together");
        test.Expect(
            !manager.HasEffect(
                StatusEffectType::COMBAT_REDUCTION) &&
                !manager.HasEffect(
                    StatusEffectType::MAX_HEALTH_BOOST),
            "Effects that reach zero expire in deterministic insertion order sweep");
    }

    {
        StatusEffectManager manager;

        manager.Apply(
            MakeEffect(
                StatusEffectType::COMBAT_BOOST,
                2,
                std::numeric_limits<int>::max(),
                std::numeric_limits<int>::max(),
                0,
                0));

        manager.Apply(
            MakeEffect(
                StatusEffectType::COMBAT_REDUCTION,
                2,
                1,
                1,
                0,
                0));

        StatusEffectModifiers combined =
            manager.GetCombinedModifiers();

        test.ExpectEqual(
            combined.meleeAccuracy,
            std::numeric_limits<int>::max(),
            "Combined modifiers clamp safely at INT_MAX to prevent overflow");
        test.ExpectEqual(
            combined.meleeStrength,
            std::numeric_limits<int>::max(),
            "Combined strength modifier clamp prevents signed overflow");
    }

    {
        StatusEffectDefinition multiStat =
            MakeEffect(
                StatusEffectType::COMBAT_BOOST,
                4,
                2,
                -3,
                5,
                7);

        test.Expect(
            IsValidStatusEffectDefinition(multiStat),
            "Validation allows mixed positive and negative modifiers across multiple stats");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int invalidPlayerID = playerID + 9999;

        Player *player = GetPlayer(world, playerID);

        CombatRatings ratingsBefore =
            player->GetCombatRatings();
        int currentHealthBefore =
            player->GetCurrentHealth();

        StatusEffectDefinition validDefinition =
            MakeEffect(
                StatusEffectType::COMBAT_BOOST,
                3,
                4,
                0,
                0,
                0);

        test.Expect(
            !world.TryApplyStatusEffect(
                invalidPlayerID,
                validDefinition),
            "Invalid player ID fails safely for status-effect application");

        test.ExpectEqual(
            player->GetCombatRatings().attackAccuracy,
            ratingsBefore.attackAccuracy,
            "Failed apply leaves derived stats unchanged");
        test.ExpectEqual(
            player->GetCurrentHealth(),
            currentHealthBefore,
            "Failed apply leaves current health unchanged");

        test.Expect(
            !world.TryRemoveStatusEffect(
                invalidPlayerID,
                StatusEffectType::COMBAT_BOOST),
            "Invalid player ID fails safely for status-effect removal");
    }

    {
        World world;

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            8,
            8,
            CombatRatings{5, 5, 5, 25});

        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        test.Expect(
            player != nullptr &&
                monster != nullptr,
            "World setup for non-player ID checks is valid");

        StatusEffectDefinition validDefinition =
            MakeEffect(
                StatusEffectType::COMBAT_BOOST,
                3,
                2,
                0,
                0,
                0);

        test.Expect(
            !world.TryApplyStatusEffect(
                monsterID,
                validDefinition),
            "Non-player entity IDs fail safely for apply");
        test.Expect(
            !world.TryRemoveStatusEffect(
                monsterID,
                StatusEffectType::COMBAT_BOOST),
            "Non-player entity IDs fail safely for remove");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        CombatRatings baseRatings =
            player->GetCombatRatings();

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    4,
                    5,
                    3,
                    2,
                    6)),
            "Applying a valid effect succeeds and refreshes derived stats");

        CombatRatings boostedRatings =
            player->GetCombatRatings();

        test.ExpectEqual(
            boostedRatings.attackAccuracy,
            baseRatings.attackAccuracy + 5,
            "Positive accuracy modifiers increase attack accuracy");
        test.ExpectEqual(
            boostedRatings.meleeStrength,
            baseRatings.meleeStrength + 3,
            "Positive strength modifiers increase melee strength");
        test.ExpectEqual(
            boostedRatings.defence,
            baseRatings.defence + 2,
            "Positive defence modifiers increase defence");
        test.ExpectEqual(
            boostedRatings.maximumHealth,
            baseRatings.maximumHealth + 6,
            "Maximum-health modifiers update derived maximum health");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        CombatRatings baseRatings =
            player->GetCombatRatings();

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeEffect(
                    StatusEffectType::COMBAT_REDUCTION,
                    3,
                    -(baseRatings.attackAccuracy + 50),
                    -(baseRatings.meleeStrength + 50),
                    -(baseRatings.defence + 50),
                    -(baseRatings.maximumHealth + 500))),
            "Strong debuff applies successfully");

        CombatRatings reduced =
            player->GetCombatRatings();

        test.ExpectEqual(
            reduced.attackAccuracy,
            0,
            "Attack accuracy cannot fall below zero");
        test.ExpectEqual(
            reduced.meleeStrength,
            0,
            "Melee strength cannot fall below zero");
        test.ExpectEqual(
            reduced.defence,
            0,
            "Melee defence cannot fall below zero");
        test.ExpectEqual(
            reduced.maximumHealth,
            1,
            "Maximum health cannot fall below one");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetInventory().AddItem(
            ItemType::WOODEN_SHIELD,
            1);

        test.Expect(
            EquipItem(
                world,
                playerID,
                ItemType::WOODEN_SHIELD),
            "Shield equips for equipment-plus-effect integration check");

        int withShieldOnly =
            player->GetCombatRatings().maximumHealth;

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeEffect(
                    StatusEffectType::MAX_HEALTH_BOOST,
                    3,
                    0,
                    0,
                    0,
                    9)),
            "Maximum-health effect applies with equipment present");

        test.ExpectEqual(
            player->GetCombatRatings().maximumHealth,
            withShieldOnly + 9,
            "Equipment bonuses and status effects combine additively");

        ItemType equippedBefore =
            player->GetEquipment().GetEquippedItem(
                EquipmentSlotType::SHIELD);

        world.TryApplyStatusEffect(
            playerID,
            MakeEffect(
                StatusEffectType::COMBAT_BOOST,
                2,
                1,
                0,
                0,
                0));

        ItemType equippedAfter =
            player->GetEquipment().GetEquippedItem(
                EquipmentSlotType::SHIELD);

        test.ExpectEqual(
            static_cast<int>(equippedAfter),
            static_cast<int>(equippedBefore),
            "Applying effects does not alter equipment state");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->ApplyDamage(30);

        int currentBefore =
            player->GetCurrentHealth();

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeEffect(
                    StatusEffectType::MAX_HEALTH_BOOST,
                    3,
                    0,
                    0,
                    0,
                    20)),
            "Maximum-health buff applies");

        test.ExpectEqual(
            player->GetCurrentHealth(),
            currentBefore,
            "Maximum-health buff does not heal current health");

        player->Heal(1000);

        test.ExpectEqual(
            player->GetCurrentHealth(),
            player->GetMaximumHealth(),
            "Setup reaches boosted health cap before clamp checks");

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeEffect(
                    StatusEffectType::MAX_HEALTH_BOOST,
                    3,
                    0,
                    0,
                    0,
                    -15)),
            "Same-type reapplication with debuff value succeeds");

        test.Expect(
            player->GetCurrentHealth() <=
                player->GetMaximumHealth(),
            "Maximum-health reduction clamps excessive current health");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        CombatRatings baseRatings =
            player->GetCombatRatings();

        StatusEffectDefinition boost =
            MakeEffect(
                StatusEffectType::COMBAT_BOOST,
                5,
                4,
                0,
                0,
                0);

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                boost),
            "Apply succeeds before explicit remove checks");

        test.Expect(
            world.TryRemoveStatusEffect(
                playerID,
                StatusEffectType::COMBAT_BOOST),
            "Removing active effect succeeds");

        test.ExpectEqual(
            player->GetCombatRatings().attackAccuracy,
            baseRatings.attackAccuracy,
            "Explicit removal restores derived stats");

        test.Expect(
            !world.TryRemoveStatusEffect(
                playerID,
                StatusEffectType::COMBAT_BOOST),
            "Removing inactive effect fails without mutation");

        player->ApplyDamage(5);
        world.TryApplyStatusEffect(
            playerID,
            MakeEffect(
                StatusEffectType::MAX_HEALTH_BOOST,
                3,
                0,
                0,
                0,
                30));
        player->Heal(1000);

        test.Expect(
            world.TryRemoveStatusEffect(
                playerID,
                StatusEffectType::MAX_HEALTH_BOOST),
            "Removing max-health effect succeeds");

        test.Expect(
            player->GetCurrentHealth() <=
                player->GetMaximumHealth(),
            "Explicit removal clamps health when maximum health falls");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        CombatRatings baseRatings =
            player->GetCombatRatings();

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    3,
                    6,
                    0,
                    0,
                    0)),
            "Duration test setup applies effect");

        test.ExpectEqual(
            player->GetCombatRatings().attackAccuracy,
            baseRatings.attackAccuracy + 6,
            "Effect is active immediately after apply");

        world.Update();
        test.ExpectEqual(
            player->GetCombatRatings().attackAccuracy,
            baseRatings.attackAccuracy + 6,
            "Duration 3 effect remains active after first completed update");

        world.Update();
        test.ExpectEqual(
            player->GetCombatRatings().attackAccuracy,
            baseRatings.attackAccuracy + 6,
            "Duration 3 effect remains active after second completed update");

        world.Update();
        test.ExpectEqual(
            player->GetCombatRatings().attackAccuracy,
            baseRatings.attackAccuracy,
            "Effect expires after the exact requested number of updates");

        world.Update();
        test.ExpectEqual(
            player->GetCombatRatings().attackAccuracy,
            baseRatings.attackAccuracy,
            "Expired effects no longer modify stats on subsequent updates");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->ApplyDamage(1);
        world.TryApplyStatusEffect(
            playerID,
            MakeEffect(
                StatusEffectType::MAX_HEALTH_BOOST,
                1,
                0,
                0,
                0,
                25));
        player->Heal(1000);

        world.Update();

        test.Expect(
            player->GetCurrentHealth() <=
                player->GetMaximumHealth(),
            "Expiry of maximum-health effects clamps current health correctly");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        CombatRatings baseRatings =
            player->GetCombatRatings();

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    2,
                    3,
                    0,
                    0,
                    0)),
            "Reapply-duration setup applies first effect");

        world.Update();

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    2,
                    3,
                    0,
                    0,
                    0)),
            "Reapplying before expiry refreshes duration");

        world.Update();
        test.ExpectEqual(
            player->GetCombatRatings().attackAccuracy,
            baseRatings.attackAccuracy + 3,
            "Refreshed effect remains active through refreshed duration");

        world.Update();
        test.ExpectEqual(
            player->GetCombatRatings().attackAccuracy,
            baseRatings.attackAccuracy,
            "Refreshed effect eventually expires at refreshed tick boundary");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    1,
                    2,
                    0,
                    0,
                    0)),
            "Multi-effect expiry setup applies short effect");
        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeEffect(
                    StatusEffectType::MAX_HEALTH_BOOST,
                    3,
                    0,
                    0,
                    0,
                    10)),
            "Multi-effect expiry setup applies long effect");

        world.Update();

        test.Expect(
            !player->GetStatusEffectManager().HasEffect(
                StatusEffectType::COMBAT_BOOST),
            "Short effect expires as expected");
        test.Expect(
            player->GetStatusEffectManager().HasEffect(
                StatusEffectType::MAX_HEALTH_BOOST),
            "Unrelated active effects remain when one effect expires");
    }

    {
        World world;
        int playerID = world.CreatePlayer();

        test.Expect(
            StartGatheringAction(
                world,
                playerID),
            "Gathering action starts for action-cancel validation");

        const Action *before =
            world.GetActionForEntity(playerID);

        test.Expect(
            before != nullptr &&
                before->GetType() == ActionType::GATHERING,
            "Gathering action is active before status mutation");

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    2,
                    1,
                    0,
                    0,
                    0)),
            "Applying status effect succeeds during gathering action");

        const Action *afterApply =
            world.GetActionForEntity(playerID);

        test.Expect(
            afterApply != nullptr &&
                afterApply->GetType() == ActionType::GATHERING,
            "Applying status effects does not cancel gathering actions");

        test.Expect(
            world.TryRemoveStatusEffect(
                playerID,
                StatusEffectType::COMBAT_BOOST),
            "Removing status effect succeeds during gathering action");

        const Action *afterRemove =
            world.GetActionForEntity(playerID);

        test.Expect(
            afterRemove != nullptr &&
                afterRemove->GetType() == ActionType::GATHERING,
            "Removing status effects does not cancel gathering actions");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        test.Expect(
            StartMeleeAction(
                world,
                attackerID,
                defenderID,
                3),
            "Melee action starts for action-cancel validation");

        const Action *before =
            world.GetActionForEntity(attackerID);

        test.Expect(
            before != nullptr &&
                before->GetType() == ActionType::MELEE_ATTACK,
            "Melee action is active before status mutation");

        test.Expect(
            world.TryApplyStatusEffect(
                attackerID,
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    2,
                    1,
                    0,
                    0,
                    0)),
            "Applying status effect succeeds during melee action");

        const Action *afterApply =
            world.GetActionForEntity(attackerID);

        test.Expect(
            afterApply != nullptr &&
                afterApply->GetType() == ActionType::MELEE_ATTACK,
            "Applying status effects does not cancel melee actions");

        test.Expect(
            world.TryRemoveStatusEffect(
                attackerID,
                StatusEffectType::COMBAT_BOOST),
            "Removing status effect succeeds during melee action");

        const Action *afterRemove =
            world.GetActionForEntity(attackerID);

        test.Expect(
            afterRemove != nullptr &&
                afterRemove->GetType() == ActionType::MELEE_ATTACK,
            "Removing status effects does not cancel melee actions");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeEffect(
                    StatusEffectType::COMBAT_BOOST,
                    3,
                    2,
                    0,
                    0,
                    0)),
            "Death-policy setup applies effect");

        player->ApplyDamage(100000);

        world.Update();

        const ActiveStatusEffect *afterDeathTick =
            player->GetStatusEffectManager().FindEffect(
                StatusEffectType::COMBAT_BOOST);

        test.Expect(
            !player->IsAlive(),
            "Player is dead for temporary death-policy validation");
        test.Expect(
            afterDeathTick != nullptr &&
                afterDeathTick->remainingTicks == 2,
            "Effects remain active through death and continue ticking");

        world.Update();
        world.Update();

        test.Expect(
            !player->GetStatusEffectManager().HasEffect(
                StatusEffectType::COMBAT_BOOST),
            "Effects expire normally after death under temporary policy");
    }

    return test.Finish();
}
