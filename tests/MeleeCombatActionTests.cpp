#include "TestSupport.h"

#include "../src/Action/ActionCancelReason.h"
#include "../src/Action/ActionType.h"
#include "../src/Core/RandomSource.h"
#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Player/Player.h"
#include "../src/Recipe/RecipeDatabase.h"
#include "../src/Recipe/RecipeType.h"
#include "../src/Stats/CombatRatings.h"
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

    void OpenFurnace(
        TestContext &test,
        World &world,
        int playerID)
    {
        CraftingStation *station =
            world.GetStationAt(14, 9);

        test.Expect(
            station != nullptr,
            "Furnace exists for melee action integration setup");

        if (station == nullptr)
        {
            return;
        }

        world.QueueStationInteraction(
            playerID,
            station->GetID());
        world.Update();
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
}

int main()
{
    TestContext test;

    {
        World world(101U);

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
                2),
            "Valid adjacent living attacker and defender start a melee action");

        const Action *action =
            world.GetActionForEntity(attackerID);

        test.Expect(
            action != nullptr,
            "Melee start creates an active action");
        test.ExpectEqual(
            static_cast<int>(action->GetType()),
            static_cast<int>(ActionType::MELEE_ATTACK),
            "Started action type is MELEE_ATTACK");
    }

    {
        World world(102U);
        int attackerID = world.CreatePlayer();
        Player *attacker = GetPlayer(world, attackerID);
        attacker->GetPosition().SetPosition(10, 10);

        test.Expect(
            !world.TryStartMeleeAttack(
                attackerID,
                attackerID,
                2),
            "Same attacker and defender ID is rejected");
    }

    {
        World world(103U);
        int defenderID = world.CreatePlayer();

        test.Expect(
            !world.TryStartMeleeAttack(
                999999,
                defenderID,
                2),
            "Missing attacker is rejected");
    }

    {
        World world(104U);
        int attackerID = world.CreatePlayer();

        test.Expect(
            !world.TryStartMeleeAttack(
                attackerID,
                999999,
                2),
            "Missing defender is rejected");
    }

    {
        World world(105U);
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        attacker->ApplyDamage(10000);

        test.Expect(
            !world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                2),
            "Dead attacker is rejected");
    }

    {
        World world(106U);
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        defender->ApplyDamage(10000);

        test.Expect(
            !world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                2),
            "Dead defender is rejected");
    }

    {
        World world(107U);
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(1, 1);
        defender->GetPosition().SetPosition(8, 8);

        test.Expect(
            !world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                2),
            "Out-of-range defender is rejected");
    }

    {
        World world(108U);
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
                3),
            "First melee action starts for existing-action validation");

        test.Expect(
            !world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                3),
            "Existing attacker action prevents starting another melee action");
    }

    {
        World world(109U);
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        test.Expect(
            !world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                0),
            "Duration below one tick is rejected by policy");
        test.Expect(
            !world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                -2),
            "Negative duration is rejected by policy");
    }

    {
        World world(110U);
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        int defenderHealthBefore =
            defender->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                2),
            "Melee action starts for timing validation");

        test.ExpectEqual(
            defender->GetCurrentHealth(),
            defenderHealthBefore,
            "Starting melee does not immediately apply damage");

        world.Update();

        test.ExpectEqual(
            defender->GetCurrentHealth(),
            defenderHealthBefore,
            "No damage is applied before completion tick");

        world.Update();

        test.Expect(
            world.GetLastMeleeAttackResult().has_value(),
            "Exactly one attack resolution occurs on completion");

        MeleeAttackResult firstResult =
            world.GetLastMeleeAttackResult().value();

        test.Expect(
            world.GetActionForEntity(attackerID) == nullptr,
            "Attacker has no active action after non-repeating melee completion");

        world.Update();

        test.Expect(
            world.GetActionForEntity(attackerID) == nullptr,
            "Non-repeating melee action does not restart automatically");
        test.Expect(
            world.GetLastMeleeAttackResult().has_value(),
            "Resolved melee result remains observable after completion");
        test.Expect(
            AreEqual(
                world.GetLastMeleeAttackResult().value(),
                firstResult),
            "No additional melee resolution occurs after completion");
    }

    {
        World world(111U);
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        int defenderHealthBefore =
            defender->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                2),
            "Melee action starts for out-of-range revalidation");

        defender->GetPosition().SetPosition(50, 50);

        AdvanceWorldTicks(world, 2);

        test.Expect(
            !world.GetLastMeleeAttackResult().has_value(),
            "Defender moving out of range before completion prevents resolution");
        test.ExpectEqual(
            defender->GetCurrentHealth(),
            defenderHealthBefore,
            "Out-of-range revalidation prevents damage");
    }

    {
        World world(1111U);
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        int defenderHealthBefore =
            defender->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                2),
            "Melee action starts for attacker out-of-range revalidation");

        attacker->GetPosition().SetPosition(50, 50);

        AdvanceWorldTicks(world, 2);

        test.Expect(
            !world.GetLastMeleeAttackResult().has_value(),
            "Attacker moving out of range before completion prevents resolution");
        test.ExpectEqual(
            defender->GetCurrentHealth(),
            defenderHealthBefore,
            "Attacker out-of-range revalidation prevents damage");
    }

    {
        World world(112U);
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        int defenderHealthBefore =
            defender->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                2),
            "Melee action starts for movement cancellation validation");

        world.CancelActionsForEntity(
            attackerID,
            ActionCancelReason::PLAYER_MOVED);

        AdvanceWorldTicks(world, 2);

        test.Expect(
            !world.GetLastMeleeAttackResult().has_value(),
            "Attacker movement cancellation prevents melee resolution");
        test.ExpectEqual(
            defender->GetCurrentHealth(),
            defenderHealthBefore,
            "Cancelled melee action does not damage defender");
    }

    {
        World world(113U);
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        int defenderHealthBefore =
            defender->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                2),
            "Melee action starts for dead-defender revalidation");

        defender->ApplyDamage(10000);

        AdvanceWorldTicks(world, 2);

        test.Expect(
            !world.GetLastMeleeAttackResult().has_value(),
            "Defender death before completion prevents melee resolution");
        test.ExpectEqual(
            defender->GetCurrentHealth(),
            0,
            "Dead defender remains at zero health");
        test.ExpectEqual(
            defenderHealthBefore,
            defender->GetMaximumHealth(),
            "Defender started this test alive at full health");
    }

    {
        World world(114U);
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        int defenderHealthBefore =
            defender->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                2),
            "Melee action starts for dead-attacker revalidation");

        attacker->ApplyDamage(10000);

        AdvanceWorldTicks(world, 2);

        test.Expect(
            !world.GetLastMeleeAttackResult().has_value(),
            "Attacker death before completion prevents melee resolution");
        test.ExpectEqual(
            defender->GetCurrentHealth(),
            defenderHealthBefore,
            "Dead attacker cannot apply damage on completion");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{999, 0, 999}));

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        const int defenderHealthBefore =
            defender->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                1),
            "Melee action starts for maximum-damage completion validation");

        world.Update();

        test.Expect(
            world.GetLastMeleeAttackResult().has_value(),
            "Maximum-damage test resolves a melee completion");

        if (world.GetLastMeleeAttackResult().has_value())
        {
            const MeleeAttackResult &result =
                world.GetLastMeleeAttackResult().value();

            test.Expect(
                result.didHit,
                "Maximum-damage test resolves a hit");
            test.ExpectEqual(
                result.rolledDamage,
                result.maximumHit,
                "Maximum-damage completion preserves rolled maximum hit");
            test.ExpectEqual(
                defenderHealthBefore - defender->GetCurrentHealth(),
                result.actualDamageApplied,
                "Maximum-damage completion applies expected health delta");
        }
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{999, 0, 999}));

        int attackerID = world.CreateMonster(
            10,
            10,
            CombatRatings{40, 80, 10, 100});

        int defenderID = world.CreateMonster(
            11,
            10,
            CombatRatings{5, 5, 5, 40});

        Monster *attacker =
            dynamic_cast<Monster *>(
                world.GetEntityByID(attackerID));

        Monster *defender =
            dynamic_cast<Monster *>(
                world.GetEntityByID(defenderID));

        test.Expect(
            attacker != nullptr && defender != nullptr,
            "Overkill test creates monster combatants");

        if (attacker != nullptr &&
            defender != nullptr)
        {
            defender->ApplyDamage(
                defender->GetCurrentHealth() - 1);

            const int defenderHealthBeforeAttack =
                defender->GetCurrentHealth();

            test.Expect(
                world.TryStartMeleeAttack(
                    attackerID,
                    defenderID,
                    1),
                "Melee action starts for overkill completion validation");

            world.Update();

            test.Expect(
                world.GetLastMeleeAttackResult().has_value(),
                "Overkill test resolves a melee completion");

            if (world.GetLastMeleeAttackResult().has_value())
            {
                const MeleeAttackResult &result =
                    world.GetLastMeleeAttackResult().value();

                test.Expect(
                    result.didHit,
                    "Overkill test resolves a hit");
                test.Expect(
                    result.rolledDamage > result.actualDamageApplied,
                    "Overkill preserves larger rolled damage than applied damage");
                test.ExpectEqual(
                    result.actualDamageApplied,
                    1,
                    "Overkill clamps applied damage to remaining health");
                test.ExpectEqual(
                    defenderHealthBeforeAttack - defender->GetCurrentHealth(),
                    result.actualDamageApplied,
                    "Overkill removes all remaining defender health exactly once");
            }
        }
    }

    {
        World world(115U);

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(12, 9);
        defender->GetPosition().SetPosition(13, 9);

        OpenFurnace(
            test,
            world,
            defenderID);

        defender->GetInventory().AddItem(ItemType::COPPER_ORE, 2);
        defender->GetInventory().AddItem(ItemType::TIN_ORE, 2);

        test.Expect(
            world.TryStartRecipeAction(
                defenderID,
                RecipeType::BRONZE_BAR),
            "Defender starts an unrelated repeating smithing action");

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                1),
            "Attacker starts melee against defender with unrelated action");

        world.Update();

        test.Expect(
            world.GetActionForEntity(defenderID) != nullptr,
            "Defender unrelated action is not cancelled by being targeted");
    }

    {
        World baseline(200U);
        World geared(200U);

        int baselineAttackerID = baseline.CreatePlayer();
        int baselineDefenderID = baseline.CreatePlayer();

        int gearedAttackerID = geared.CreatePlayer();
        int gearedDefenderID = geared.CreatePlayer();

        Player *baselineAttacker = GetPlayer(baseline, baselineAttackerID);
        Player *baselineDefender = GetPlayer(baseline, baselineDefenderID);
        Player *gearedAttacker = GetPlayer(geared, gearedAttackerID);
        Player *gearedDefender = GetPlayer(geared, gearedDefenderID);

        baselineAttacker->GetPosition().SetPosition(10, 10);
        baselineDefender->GetPosition().SetPosition(11, 10);
        gearedAttacker->GetPosition().SetPosition(10, 10);
        gearedDefender->GetPosition().SetPosition(11, 10);

        test.Expect(
            EquipItem(
                geared,
                gearedAttackerID,
                ItemType::BRONZE_SWORD),
            "Bronze sword equip succeeds for melee integration test");

        test.Expect(
            EquipItem(
                geared,
                gearedDefenderID,
                ItemType::WOODEN_SHIELD),
            "Wooden shield equip succeeds for melee integration test");

        CombatRatings gearedAttackerRatingsBefore =
            gearedAttacker->GetCombatRatings();
        CombatRatings gearedDefenderRatingsBefore =
            gearedDefender->GetCombatRatings();

        int defenderHealthBefore =
            gearedDefender->GetCurrentHealth();

        test.Expect(
            baseline.TryStartMeleeAttack(
                baselineAttackerID,
                baselineDefenderID,
                1),
            "Baseline melee action starts");

        test.Expect(
            geared.TryStartMeleeAttack(
                gearedAttackerID,
                gearedDefenderID,
                1),
            "Geared melee action starts");

        baseline.Update();
        geared.Update();

        test.Expect(
            baseline.GetLastMeleeAttackResult().has_value(),
            "Baseline melee action resolves");
        test.Expect(
            geared.GetLastMeleeAttackResult().has_value(),
            "Geared melee action resolves");

        const MeleeAttackResult &baselineResult =
            baseline.GetLastMeleeAttackResult().value();

        const MeleeAttackResult &gearedResult =
            geared.GetLastMeleeAttackResult().value();

        test.Expect(
            gearedResult.attackerMaximumRoll > baselineResult.attackerMaximumRoll,
            "Bronze sword increases attacker maximum roll through ratings");
        test.Expect(
            gearedResult.maximumHit >= baselineResult.maximumHit,
            "Bronze sword can increase maximum hit through ratings");
        test.Expect(
            gearedResult.defenderMaximumRoll > baselineResult.defenderMaximumRoll,
            "Wooden shield increases defender roll through ratings");

        test.ExpectEqual(
            defenderHealthBefore - gearedDefender->GetCurrentHealth(),
            gearedResult.actualDamageApplied,
            "Actual defender health removed matches resolved actual damage");

        test.Expect(
            AreEqual(
                gearedAttacker->GetCombatRatings(),
                gearedAttackerRatingsBefore),
            "Attacker ratings remain unchanged by melee resolution");
        test.Expect(
            AreEqual(
                gearedDefender->GetCombatRatings(),
                gearedDefenderRatingsBefore),
            "Defender ratings remain unchanged by melee resolution");
    }

    {
        World first(300U);
        World second(300U);

        int firstAttackerID = first.CreatePlayer();
        int firstDefenderID = first.CreatePlayer();
        int secondAttackerID = second.CreatePlayer();
        int secondDefenderID = second.CreatePlayer();

        Player *firstAttacker = GetPlayer(first, firstAttackerID);
        Player *firstDefender = GetPlayer(first, firstDefenderID);
        Player *secondAttacker = GetPlayer(second, secondAttackerID);
        Player *secondDefender = GetPlayer(second, secondDefenderID);

        firstAttacker->GetPosition().SetPosition(10, 10);
        firstDefender->GetPosition().SetPosition(11, 10);
        secondAttacker->GetPosition().SetPosition(10, 10);
        secondDefender->GetPosition().SetPosition(11, 10);

        for (int index = 0; index < 6; ++index)
        {
            test.Expect(
                first.TryStartMeleeAttack(
                    firstAttackerID,
                    firstDefenderID,
                    1),
                "First seeded world can start repeated melee actions");

            test.Expect(
                second.TryStartMeleeAttack(
                    secondAttackerID,
                    secondDefenderID,
                    1),
                "Second seeded world can start repeated melee actions");

            first.Update();
            second.Update();

            test.Expect(
                first.GetLastMeleeAttackResult().has_value() &&
                    second.GetLastMeleeAttackResult().has_value(),
                "Both seeded worlds resolve melee actions");

            test.Expect(
                AreEqual(
                    first.GetLastMeleeAttackResult().value(),
                    second.GetLastMeleeAttackResult().value()),
                "Same world seed produces identical melee result sequences");

            firstDefender->RestoreHealthToFull();
            secondDefender->RestoreHealthToFull();
        }
    }

    {
        World first(301U);
        World second(999U);

        int firstAttackerID = first.CreatePlayer();
        int firstDefenderID = first.CreatePlayer();
        int secondAttackerID = second.CreatePlayer();
        int secondDefenderID = second.CreatePlayer();

        Player *firstAttacker = GetPlayer(first, firstAttackerID);
        Player *firstDefender = GetPlayer(first, firstDefenderID);
        Player *secondAttacker = GetPlayer(second, secondAttackerID);
        Player *secondDefender = GetPlayer(second, secondDefenderID);

        firstAttacker->GetPosition().SetPosition(10, 10);
        firstDefender->GetPosition().SetPosition(11, 10);
        secondAttacker->GetPosition().SetPosition(10, 10);
        secondDefender->GetPosition().SetPosition(11, 10);

        bool diverged = false;

        for (int index = 0; index < 12; ++index)
        {
            test.Expect(
                first.TryStartMeleeAttack(
                    firstAttackerID,
                    firstDefenderID,
                    1),
                "First differing-seed world can start repeated melee actions");

            test.Expect(
                second.TryStartMeleeAttack(
                    secondAttackerID,
                    secondDefenderID,
                    1),
                "Second differing-seed world can start repeated melee actions");

            first.Update();
            second.Update();

            const MeleeAttackResult &firstResult =
                first.GetLastMeleeAttackResult().value();

            const MeleeAttackResult &secondResult =
                second.GetLastMeleeAttackResult().value();

            if (!AreEqual(firstResult, secondResult))
            {
                diverged = true;
                break;
            }

            firstDefender->RestoreHealthToFull();
            secondDefender->RestoreHealthToFull();
        }

        test.Expect(
            diverged,
            "Different world seeds normally diverge across melee result sequences");
    }

    return test.Finish();
}
