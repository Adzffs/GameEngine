#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Action/ActionCancelReason.h"
#include "../src/Action/ActionType.h"
#include "../src/Core/RandomSource.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Player/Player.h"
#include "../src/Recipe/RecipeType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/Event/EntityDiedEvent.h"
#include "../src/World/Object/Station/CraftingStation.h"
#include "../src/World/World.h"

#include <memory>
#include <type_traits>
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
}

int main()
{
    TestContext test;

    static_assert(
        std::is_same_v<
            decltype(std::declval<const World &>().GetEntityDiedEvents()),
            const std::vector<EntityDiedEvent> &>,
        "Death-event collection must be exposed as a const reference");

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{5, 1, 1}));

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            1);

        world.Update();

        test.Expect(
            defender->GetCurrentHealth() > 0,
            "Nonlethal damage keeps defender alive");
        test.Expect(
            world.GetEntityDiedEvents().empty(),
            "Nonlethal damage creates no death event");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{1, 3, 1}));

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            1);

        world.Update();

        test.Expect(
            world.GetEntityDiedEvents().empty(),
            "Miss creates no death event");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{5, 1, 0}));

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            1);

        world.Update();

        test.Expect(
            world.GetEntityDiedEvents().empty(),
            "Zero-damage hit creates no death event");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1}));

        int playerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1));

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(10, 10);

        world.TryStartMeleeAttack(
            playerID,
            monsterID,
            1);

        world.Update();

        const std::vector<EntityDiedEvent> &events =
            world.GetEntityDiedEvents();

        test.ExpectEqual(
            static_cast<int>(events.size()),
            1,
            "Killing damage creates exactly one death event");

        if (!events.empty())
        {
            test.ExpectEqual(
                events[0].deadEntityID,
                monsterID,
                "Death event records the dead entity");
            test.ExpectEqual(
                events[0].killerEntityID,
                playerID,
                "Death event records the killer entity");
            test.ExpectEqual(
                events[0].deathTick,
                world.GetCurrentTick(),
                "Death event records the authoritative world tick");
        }

        test.Expect(
            world.GetEntityByID(monsterID) != nullptr,
            "Dead monster remains in EntityManager");

        world.Update();

        test.Expect(
            world.GetEntityDiedEvents().empty(),
            "Death events are cleared deterministically on the next update");

        world.Update();

        test.Expect(
            world.GetEntityDiedEvents().empty(),
            "Repeated world updates do not duplicate death events");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1, 9, 1, 1}));

        int firstKillerID = world.CreatePlayer();
        int secondKillerID = world.CreatePlayer();
        int monsterID = world.CreateMonster(
            11,
            10,
            MakeRatings(1, 1, 1, 1));

        Player *firstKiller =
            GetPlayer(world, firstKillerID);
        Player *secondKiller =
            GetPlayer(world, secondKillerID);
        Monster *monster =
            GetMonster(world, monsterID);

        firstKiller->GetPosition().SetPosition(10, 10);
        secondKiller->GetPosition().SetPosition(12, 10);

        world.TryStartMeleeAttack(
            firstKillerID,
            monsterID,
            1);

        world.Update();

        const std::vector<EntityDiedEvent> &firstDeathEvents =
            world.GetEntityDiedEvents();

        test.ExpectEqual(
            static_cast<int>(firstDeathEvents.size()),
            1,
            "First monster death produces exactly one event");

        int firstDeathTick = -1;

        if (!firstDeathEvents.empty())
        {
            firstDeathTick =
                firstDeathEvents[0].deathTick;

            test.ExpectEqual(
                firstDeathEvents[0].deadEntityID,
                monsterID,
                "First death records the monster ID");
            test.ExpectEqual(
                firstDeathEvents[0].killerEntityID,
                firstKillerID,
                "First death records the first killer");
        }

        world.Update();

        test.Expect(
            world.GetEntityDiedEvents().empty(),
            "First death event clears on next update without duplication");

        test.Expect(
            monster != nullptr,
            "Monster exists for repeated lifecycle validation");

        if (monster != nullptr)
        {
            monster->RestoreHealthToFull();
        }

        world.Update();

        test.Expect(
            world.GetEntityDiedEvents().empty(),
            "Revival re-arm update creates no death event");

        world.TryStartMeleeAttack(
            secondKillerID,
            monsterID,
            1);

        world.Update();

        const std::vector<EntityDiedEvent> &secondDeathEvents =
            world.GetEntityDiedEvents();

        test.ExpectEqual(
            static_cast<int>(secondDeathEvents.size()),
            1,
            "Second death after revival produces exactly one new event");

        if (!secondDeathEvents.empty())
        {
            test.ExpectEqual(
                secondDeathEvents[0].deadEntityID,
                monsterID,
                "Second death records the same monster ID");
            test.ExpectEqual(
                secondDeathEvents[0].killerEntityID,
                secondKillerID,
                "Second death records the second killer");
            test.Expect(
                secondDeathEvents[0].deathTick >
                    firstDeathTick,
                "Second death event records a later world tick");
        }

        world.Update();

        test.Expect(
            world.GetEntityDiedEvents().empty(),
            "Further dead ticks do not duplicate the second death event");
    }

    {
        World world;

        int playerID = world.CreatePlayer();

        Player *player = GetPlayer(world, playerID);
        player->ApplyDamage(100000);

        world.Update();

        const std::vector<EntityDiedEvent> &events =
            world.GetEntityDiedEvents();

        test.ExpectEqual(
            static_cast<int>(events.size()),
            1,
            "Dead-combatant cleanup discovers an out-of-combat death once");

        if (!events.empty())
        {
            test.ExpectEqual(
                events[0].deadEntityID,
                playerID,
                "Cleanup death event records the dead entity");
            test.ExpectEqual(
                events[0].killerEntityID,
                EntityDiedEvent::InvalidKillerEntityID,
                "Cleanup death event uses invalid killer when no damage source is known");
        }

        world.Update();

        test.Expect(
            world.GetEntityDiedEvents().empty(),
            "Dead-combatant cleanup remains idempotent across updates");
        test.Expect(
            world.GetEntityByID(playerID) != nullptr,
            "Dead player remains in EntityManager");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1}));

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            2);

        world.CancelActionsForEntity(
            attackerID,
            ActionCancelReason::INTERFACE_CLOSED);

        AdvanceWorldTicks(world, 2);

        test.Expect(
            world.GetEntityDiedEvents().empty(),
            "Cancelled attacks create no death event");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1}));

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            1);

        defender->GetPosition().SetPosition(40, 40);

        world.Update();

        test.Expect(
            world.GetEntityDiedEvents().empty(),
            "Invalidated completion creates no death event");
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1}));

        int defenderID = world.CreatePlayer();
        int attackerID = world.CreateMonster(
            11,
            10,
            MakeRatings(10, 15, 1, 30));

        Player *defender = GetPlayer(world, defenderID);
        Monster *attacker = GetMonster(world, attackerID);

        attacker->GetPosition().SetPosition(11, 10);
        defender->GetPosition().SetPosition(10, 10);
        defender->ApplyDamage(defender->GetCurrentHealth() - 1);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            1);

        world.Update();

        const std::vector<EntityDiedEvent> &events =
            world.GetEntityDiedEvents();

        test.ExpectEqual(
            static_cast<int>(events.size()),
            1,
            "Monster killing player creates one death event");

        if (!events.empty())
        {
            test.ExpectEqual(
                events[0].deadEntityID,
                defenderID,
                "Monster kill records dead player");
            test.ExpectEqual(
                events[0].killerEntityID,
                attackerID,
                "Monster kill records monster as killer");
        }
    }

    {
        World world(std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1}));

        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        int observerID = world.CreatePlayer();
        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);
        Player *observer = GetPlayer(world, observerID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);
        observer->GetPosition().SetPosition(13, 9);

        OpenFurnace(test, world, observerID);

        observer->GetInventory().AddItem(ItemType::COPPER_ORE, 2);
        observer->GetInventory().AddItem(ItemType::TIN_ORE, 2);

        world.TryStartRecipeAction(
            observerID,
            RecipeType::BRONZE_BAR);

        defender->ApplyDamage(defender->GetCurrentHealth() - 1);

        world.TryStartMeleeEngagement(
            attackerID,
            defenderID,
            1);

        world.TryStartMeleeEngagement(
            defenderID,
            attackerID,
            1);

        world.Update();

        const Action *attackerAction =
            world.GetActionForEntity(attackerID);
        const Action *defenderAction =
            world.GetActionForEntity(defenderID);
        const Action *observerAction =
            world.GetActionForEntity(observerID);

        test.Expect(
            attackerAction == nullptr,
            "Death cleanup cancels reciprocal melee action from the killer's side");
        test.Expect(
            defenderAction == nullptr,
            "Death cleanup cancels reciprocal melee action from the dead side");
        test.Expect(
            observerAction != nullptr &&
                observerAction->GetType() == ActionType::RECIPE,
            "Unrelated action remains active during combat death cleanup");
    }

    return test.Finish();
}
