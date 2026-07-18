#include "TestSupport.h"

#include <functional>

#include "../src/Action/Action.h"
#include "../src/Action/ActionCancelReason.h"
#include "../src/Action/ActionType.h"
#include "../src/Combat/Combatant.h"
#include "../src/Entity/Entity.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Movement/MovementRequest.h"
#include "../src/Player/Player.h"
#include "../src/Recipe/RecipeType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/Object/Station/CraftingStation.h"
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
        CraftingStation *station =
            world.GetStationAt(14, 9);

        test.Expect(
            station != nullptr,
            "Furnace exists for unrelated-action validation");

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

    int FindSeedForResult(
        const std::function<bool(const MeleeAttackResult &)> &predicate)
    {
        for (int seed = 1; seed <= 5000; ++seed)
        {
            World world(static_cast<unsigned int>(seed));

            int attackerID = world.CreatePlayer();
            int defenderID = world.CreatePlayer();

            Player *attacker = GetPlayer(world, attackerID);
            Player *defender = GetPlayer(world, defenderID);

            attacker->GetPosition().SetPosition(10, 10);
            defender->GetPosition().SetPosition(11, 10);

            if (!world.TryStartMeleeAttack(
                    attackerID,
                    defenderID,
                    1))
            {
                continue;
            }

            world.Update();

            if (!world.GetLastMeleeAttackResult().has_value())
            {
                continue;
            }

            const MeleeAttackResult &result =
                world.GetLastMeleeAttackResult().value();

            if (predicate(result))
            {
                return seed;
            }
        }

        return -1;
    }
}

int main()
{
    TestContext test;

    {
        World world(100U);

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
            "One-shot melee starts successfully");

        const Action *action =
            world.GetActionForEntity(attackerID);

        test.Expect(
            action != nullptr,
            "One-shot melee creates an active action");
        test.Expect(
            action != nullptr && !action->IsRepeating(),
            "One-shot melee remains non-repeating");
    }

    {
        World world(101U);

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        int defenderHealthBefore = defender->GetCurrentHealth();

        test.Expect(
            world.TryStartMeleeEngagement(
                attackerID,
                defenderID,
                2),
            "Repeating engagement starts successfully");

        const Action *action =
            world.GetActionForEntity(attackerID);

        test.Expect(
            action != nullptr,
            "Repeating engagement creates an active action");
        test.Expect(
            action != nullptr && action->IsRepeating(),
            "Repeating engagement is marked repeating");
        test.ExpectEqual(
            defender->GetCurrentHealth(),
            defenderHealthBefore,
            "No damage is applied before the completion tick");
        test.ExpectNear(
            action != nullptr ? action->GetProgress() : -1.0f,
            0.0f,
            0.001f,
            "Fresh engagement begins at zero progress");

        world.Update();

        test.Expect(
            !world.GetLastMeleeAttackResult().has_value(),
            "No resolution occurs before the first completion interval");

        world.Update();

        test.Expect(
            world.GetLastMeleeAttackResult().has_value(),
            "One resolution occurs at the first completion interval");

        const MeleeAttackResult firstResult =
            world.GetLastMeleeAttackResult().value();

        action = world.GetActionForEntity(attackerID);

        test.Expect(
            action != nullptr,
            "Repeating engagement restarts after completion");
        test.Expect(
            action != nullptr && action->IsRepeating(),
            "Restarted engagement remains repeating");
        test.ExpectNear(
            action != nullptr ? action->GetProgress() : -1.0f,
            0.0f,
            0.001f,
            "Restarted engagement resets progress to zero");
        test.Expect(
            !world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                2),
            "Only one active attacker action exists while engagement is running");

        world.Update();

        test.Expect(
            AreEqual(
                world.GetLastMeleeAttackResult().value(),
                firstResult),
            "No extra melee resolution happens before the next completion interval");
    }

    {
        int missSeed = FindSeedForResult(
            [](const MeleeAttackResult &result)
            {
                return !result.didHit;
            });

        test.Expect(
            missSeed >= 0,
            "A deterministic miss seed can be found");

        if (missSeed >= 0)
        {
            World world(static_cast<unsigned int>(missSeed));

            int attackerID = world.CreatePlayer();
            int defenderID = world.CreatePlayer();

            Player *attacker = GetPlayer(world, attackerID);
            Player *defender = GetPlayer(world, defenderID);

            attacker->GetPosition().SetPosition(10, 10);
            defender->GetPosition().SetPosition(11, 10);

            test.Expect(
                world.TryStartMeleeEngagement(
                    attackerID,
                    defenderID,
                    1),
                "Repeating engagement starts for miss validation");

            world.Update();

            test.Expect(
                world.GetLastMeleeAttackResult().has_value() &&
                    !world.GetLastMeleeAttackResult().value().didHit,
                "A miss is resolved deterministically");
            test.Expect(
                world.GetActionForEntity(attackerID) != nullptr,
                "A miss continues the engagement");
        }
    }

    {
        int zeroDamageSeed = FindSeedForResult(
            [](const MeleeAttackResult &result)
            {
                return result.didHit &&
                       result.actualDamageApplied == 0;
            });

        test.Expect(
            zeroDamageSeed >= 0,
            "A deterministic zero-damage hit seed can be found");

        if (zeroDamageSeed >= 0)
        {
            World world(static_cast<unsigned int>(zeroDamageSeed));

            int attackerID = world.CreatePlayer();
            int defenderID = world.CreatePlayer();

            Player *attacker = GetPlayer(world, attackerID);
            Player *defender = GetPlayer(world, defenderID);

            attacker->GetPosition().SetPosition(10, 10);
            defender->GetPosition().SetPosition(11, 10);

            test.Expect(
                world.TryStartMeleeEngagement(
                    attackerID,
                    defenderID,
                    1),
                "Repeating engagement starts for zero-damage validation");

            world.Update();

            test.Expect(
                world.GetLastMeleeAttackResult().has_value() &&
                    world.GetLastMeleeAttackResult().value().didHit &&
                    world.GetLastMeleeAttackResult().value().actualDamageApplied == 0,
                "A zero-damage hit is resolved deterministically");
            test.Expect(
                world.GetActionForEntity(attackerID) != nullptr,
                "A zero-damage hit continues the engagement");
        }
    }

    {
        World world(202U);

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        test.Expect(
            world.TryStartMeleeEngagement(
                attackerID,
                defenderID,
                1),
            "Engagement starts for attacker-death validation");

        world.Update();

        attacker->ApplyDamage(10000);

        world.Update();

        test.Expect(
            world.GetActionForEntity(attackerID) == nullptr,
            "Attacker death stops repetition");
    }

    {
        World world(203U);

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        test.Expect(
            world.TryStartMeleeEngagement(
                attackerID,
                defenderID,
                1),
            "Engagement starts for defender-movement validation");

        world.Update();

        defender->GetPosition().SetPosition(50, 50);

        world.Update();

        test.Expect(
            world.GetActionForEntity(attackerID) == nullptr,
            "Defender leaving range stops repetition");
    }

    {
        World world(204U);

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        test.Expect(
            world.TryStartMeleeEngagement(
                attackerID,
                defenderID,
                1),
            "Engagement starts for attacker-movement validation");

        world.Update();

        world.QueueMovementRequest(
            MovementRequest(
                attackerID,
                1,
                0));

        world.Update();

        test.Expect(
            world.GetActionForEntity(attackerID) == nullptr,
            "Attacker movement stops repetition");
    }

    {
        World world(205U);

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        test.Expect(
            world.TryStartMeleeEngagement(
                attackerID,
                defenderID,
                2),
            "Engagement starts for explicit cancellation validation");

        world.Update();

        world.CancelActionsForEntity(
            attackerID,
            ActionCancelReason::INTERFACE_CLOSED);

        int defenderHealthBefore = defender->GetCurrentHealth();

        world.Update();

        test.ExpectEqual(
            defender->GetCurrentHealth(),
            defenderHealthBefore,
            "Explicit cancellation prevents a delayed final hit");
        test.Expect(
            world.GetActionForEntity(attackerID) == nullptr,
            "Explicit cancellation clears the active engagement");
    }

    {
        World world(206U);

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreateMonster(
            20,
            20,
            CombatRatings{5, 5, 5, 10});

        Player *attacker = GetPlayer(world, attackerID);
        Monster *defender = GetMonster(world, defenderID);

        attacker->GetPosition().SetPosition(19, 20);
        defender->GetPosition().SetPosition(20, 20);

        test.Expect(
            world.TryStartMeleeEngagement(
                attackerID,
                defenderID,
                1),
            "Player/Monster engagement starts");

        world.Update();

        test.Expect(
            world.GetLastMeleeAttackResult().has_value(),
            "Player attacking Monster resolves");
    }

    {
        World world(207U);

        int defenderID = world.CreatePlayer();
        int attackerID = world.CreateMonster(
            20,
            20,
            CombatRatings{5, 5, 5, 10});

        Player *defender = GetPlayer(world, defenderID);
        Monster *attacker = GetMonster(world, attackerID);

        attacker->GetPosition().SetPosition(20, 20);
        defender->GetPosition().SetPosition(21, 20);

        test.Expect(
            world.TryStartMeleeEngagement(
                attackerID,
                defenderID,
                1),
            "Monster/Player engagement starts");

        world.Update();

        test.Expect(
            world.GetLastMeleeAttackResult().has_value(),
            "Monster attacking Player resolves");
    }

    {
        World world(208U);

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

        defender->GetInventory().AddItem(ItemType::COPPER_ORE, 1);
        defender->GetInventory().AddItem(ItemType::TIN_ORE, 1);

        test.Expect(
            world.TryStartRecipeAction(
                defenderID,
                RecipeType::BRONZE_BAR),
            "Defender can start an unrelated action before engagement");

        test.Expect(
            world.TryStartMeleeEngagement(
                attackerID,
                defenderID,
                1),
            "Engagement starts against a defender with an unrelated action");

        world.Update();

        test.Expect(
            world.GetActionForEntity(defenderID) != nullptr,
            "Defender's unrelated action stays active");
    }

    {
        World repeatedWorld(209U);
        World referenceWorld(209U);

        int repeatedAttackerID = repeatedWorld.CreatePlayer();
        int repeatedDefenderID = repeatedWorld.CreatePlayer();
        int referenceAttackerID = referenceWorld.CreatePlayer();
        int referenceDefenderID = referenceWorld.CreatePlayer();

        Player *repeatedAttacker = GetPlayer(repeatedWorld, repeatedAttackerID);
        Player *repeatedDefender = GetPlayer(repeatedWorld, repeatedDefenderID);
        Player *referenceAttacker = GetPlayer(referenceWorld, referenceAttackerID);
        Player *referenceDefender = GetPlayer(referenceWorld, referenceDefenderID);

        repeatedAttacker->GetPosition().SetPosition(10, 10);
        repeatedDefender->GetPosition().SetPosition(11, 10);
        referenceAttacker->GetPosition().SetPosition(10, 10);
        referenceDefender->GetPosition().SetPosition(11, 10);

        test.Expect(
            repeatedWorld.TryStartMeleeEngagement(
                repeatedAttackerID,
                repeatedDefenderID,
                1),
            "Repeating world starts engagement for RNG sequence validation");
        test.Expect(
            referenceWorld.TryStartMeleeAttack(
                referenceAttackerID,
                referenceDefenderID,
                1),
            "Reference world starts first one-shot attack for RNG validation");

        repeatedWorld.Update();
        referenceWorld.Update();

        const MeleeAttackResult repeatedFirst =
            repeatedWorld.GetLastMeleeAttackResult().value();
        const MeleeAttackResult referenceFirst =
            referenceWorld.GetLastMeleeAttackResult().value();

        test.Expect(
            AreEqual(repeatedFirst, referenceFirst),
            "First attack matches the reference RNG sequence");

        test.Expect(
            repeatedWorld.GetActionForEntity(repeatedAttackerID) != nullptr,
            "Repeating world keeps the action active after the first resolution");

        test.Expect(
            referenceWorld.TryStartMeleeAttack(
                referenceAttackerID,
                referenceDefenderID,
                1),
            "Reference world starts second one-shot attack for RNG validation");

        repeatedWorld.Update();
        referenceWorld.Update();

        const MeleeAttackResult repeatedSecond =
            repeatedWorld.GetLastMeleeAttackResult().value();
        const MeleeAttackResult referenceSecond =
            referenceWorld.GetLastMeleeAttackResult().value();

        test.Expect(
            AreEqual(repeatedSecond, referenceSecond),
            "Repeated engagement advances the RNG sequence between attacks");
    }

    return test.Finish();
}
